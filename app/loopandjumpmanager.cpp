#include "loopandjumpmanager.h"
#include <yaml-cpp/yaml.h>
#include <QTimer>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using namespace dj;

struct JumpOrLoopData {
  loop_and_jump_type_t type = dj::loop_and_jump_type_t::JUMP_BEAT;
  int start = 0;
  int end = 0; //XXX deal with it
  double length = 0;
};

struct LoopAndJumpPlayerData {
  int work_id = 0;
  int frame = 0;
  int jump_next = 0;
  bool clear_next = false; //do we clear the next jump input
  QHash<int, JumpOrLoopData> data;
  djaudio::BeatBufferPtr beats;
};

namespace {
  int closest_beat(int frame, LoopAndJumpPlayerData * pdata) {
    if (pdata->beats && pdata->beats->size() > 2) {
      if (frame <= pdata->beats->at(0)) {
        return 0;
      } else if (frame >= pdata->beats->back()) {
        return pdata->beats->size() - 1;
      } else {
        for (unsigned int i = 1; i < pdata->beats->size(); i++) {
          const int start = pdata->beats->at(i - 1);
          const int end = pdata->beats->at(i);
          if (frame >= start && frame < end) {
            //closer to which side?
            if (abs(frame - start) < abs(end - frame)) {
              return i - 1;
            } else {
              return i;
            }
          }
        }
      }
    }
    return -1;
  }
}

LoopAndJumpManager::LoopAndJumpManager(QObject * parent) :
  QObject(parent)
{
  for (int i = 0; i < 2; i++) {
    LoopAndJumpPlayerData * data = new LoopAndJumpPlayerData;
    mPlayerData.push_back(data);
  }
}

void LoopAndJumpManager::setDB(DB * db) {
  mDB = db;
}

void LoopAndJumpManager::saveData() {
  for (int i = 0; i < mPlayerData.size(); i++)
    saveData(i);
}

void LoopAndJumpManager::playerTrigger(int player, QString name) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  if (name == "jump") {
    playerSetValueInt(player, "jump", mPlayerData[player]->jump_next);
  } else if (name == "jump_clear_next") {
    mPlayerData[player]->clear_next = true;
  } else if (name == "jump_clear_next_abort") {
    mPlayerData[player]->clear_next = false;
  } else if (name == "jump_new") { //use the 'next' but reset it
    clearEntry(player, mPlayerData[player]->jump_next);
    playerSetValueInt(player, "jump", mPlayerData[player]->jump_next);
  } else if (name == "loop_off") {
    emit(playerValueChangedBool(player, "loop", false));
  }
}

void LoopAndJumpManager::playerSetValueInt(int player, QString name, int v) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  LoopAndJumpPlayerData * pdata = mPlayerData[player];
  
  if (name == "loading_work") {
    saveData(player);
    pdata->beats = nullptr;
    pdata->data.clear();
    pdata->work_id = v;
    pdata->jump_next = 0;
    emit(entriesCleared(player));
  } else if (name == "position_frame") {
    pdata->frame = v;
  } else if (name == "loop_length") {
    double beats = pow(2.0, static_cast<double>(v));
    emit(playerValueChangedDouble(player, "loop_length_beats", beats));
  } else if (name == "jump_next") {
    if (v < 0)
      return;
    pdata->jump_next = v;
  } else if (name == "jump_clear") {
    clearEntry(player, v);
    pdata->clear_next = false;
  } else if (name == "jump") {
    pdata->jump_next = v;
    if (pdata->clear_next) {
      clearEntry(player, v);
      pdata->clear_next = false;
    } else {
      auto it = pdata->data.find(v);
      if (it != pdata->data.end()) {
        int index = it->start;
        switch (it->type) {
          case loop_and_jump_type_t::LOOP_BEAT:
          case loop_and_jump_type_t::JUMP_BEAT:
            if (!pdata->beats || index < 0 || pdata->beats->size() <= static_cast<unsigned int>(index)) {
              cerr << "stored beat index[" << index << "] out of range" << endl;
              return;
            }
            index = pdata->beats->at(index);
            break;
          default: //otherwise it is a frame index already
            break;
        }
        emit(playerValueChangedInt(player, "seek_frame", index));
      } else {
        //find the closest beat
        //XXX make snap to be configurable!
        int frame = pdata->frame;
        int beat = closest_beat(frame, pdata);
        if (beat >= 0)
          frame = pdata->beats->at(beat);

        JumpOrLoopData data;
        data.type = loop_and_jump_type_t::JUMP_BEAT; //XXX make configurable
        switch (data.type) {
          case loop_and_jump_type_t::LOOP_BEAT:
          case loop_and_jump_type_t::JUMP_BEAT:
            data.start = beat;
            data.end = beat;
            break;

          default:
            data.start = frame;
            data.end = frame;
            break;
        }
        pdata->data[v] = data;
        emit(entryUpdated(player, data.type, v, data.start, data.end));
      }
    }
  }
}

void LoopAndJumpManager::playerLoad(int player, djaudio::AudioBufferPtr  audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  mPlayerData[player]->beats = beat_buffer;
  if (beat_buffer || audio_buffer) {
    mPlayerData[player]->data.clear();
    mPlayerData[player]->frame = 0;
    mPlayerData[player]->jump_next = 0;
    mPlayerData[player]->clear_next = false;
    emit(entriesCleared(player));

    //wait to load data so that other objects get the beat buffer
    //workaround for QTimer::singleShot not having lambda connections
    QTimer * t = new QTimer;
    t->setSingleShot(true);
    connect(t, &QTimer::timeout, [player, this, t] {
      loadData(player);
      t->deleteLater();
    });
    t->start(1);
  }
}

void LoopAndJumpManager::clearEntry(int player, int entry_index) {
  LoopAndJumpPlayerData * pdata = mPlayerData[player];
  auto it = pdata->data.find(entry_index);
  if (it != pdata->data.end())
    pdata->data.erase(it);
  emit(entryCleared(player, entry_index));
}

void LoopAndJumpManager::saveData(int player) {
  if (!mDB || mPlayerData[player]->work_id == 0)
    return;
  mDB->work_jump_data(mPlayerData[player]->work_id, yamlData(player));
}

void LoopAndJumpManager::loadData(int player) {
  if (!mDB || mPlayerData[player]->work_id == 0)
    return;
  const static std::map<std::string, loop_and_jump_type_t> type_map = {
    {"jump_beat", loop_and_jump_type_t::JUMP_BEAT},
    {"jump_frame", loop_and_jump_type_t::JUMP_FRAME},
    {"loop_beat", loop_and_jump_type_t::LOOP_BEAT},
    {"loop_frame", loop_and_jump_type_t::LOOP_FRAME}
  };
  try {
    LoopAndJumpPlayerData * pdata = mPlayerData[player];
    QString data = mDB->work_jump_data(mPlayerData[player]->work_id);
    if (data.size()) {
      YAML::Node root = YAML::Load(data.toStdString());
      for (unsigned int i = 0; i < root.size(); i++) {
        YAML::Node entry = root[i];

        int index = entry["index"].as<int>();
        JumpOrLoopData data;
        data.start = entry["start"].as<int>();
        data.end = entry["end"].as<int>();

        std::string t = entry["type"].as<std::string>();
        auto it = type_map.find(t);
        if (it == type_map.end()) {
          cerr << t << " is not a recognized jump/loop type name" << endl;
          continue;
        }
        data.type = it->second;

        pdata->data[index] = data;
        emit(entryUpdated(player, data.type, index, data.start, data.end));
      }
    }
  } catch (YAML::Exception& e) {
    cerr << "exception parsing yaml data: " << e.what() << endl;
  } catch (std::runtime_error& e) {
    cerr << "exception querying work jump data: " << e.what() << endl;
  }
}

QString LoopAndJumpManager::yamlData(int player) {
  LoopAndJumpPlayerData * pdata = mPlayerData[player];
  if (pdata->data.size() == 0)
    return QString();

  YAML::Node root;
  for (QHash<int, JumpOrLoopData>::iterator it = pdata->data.begin(); it != pdata->data.end(); it++) {
    YAML::Node entry;

    JumpOrLoopData * data = &it.value();
    entry["index"] = it.key();
    switch (data->type) {
      case JUMP_BEAT:
        entry["type"] = "jump_beat";
        break;
      case JUMP_FRAME:
        entry["type"] = "jump_frame";
        break;
      case LOOP_BEAT:
        entry["type"] = "loop_beat";
        break;
      case LOOP_FRAME:
        entry["type"] = "loop_frame";
        break;
    }
    entry["start"] = data->start;
    entry["end"] = data->end;
    root.push_back(entry);
  }
  return QString::fromStdString(YAML::Dump(root));
}

