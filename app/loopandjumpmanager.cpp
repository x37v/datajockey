#include "loopandjumpmanager.h"

struct JumpOrLoopData {
  enum loop_length_t { BEATS, FRAMES };

  dj::loop_and_jump_type_t type = dj::loop_and_jump_type_t::JUMP;
  int frame = 0;

  //only used for loops
  loop_length_t length_type = BEATS;
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

LoopAndJumpManager::LoopAndJumpManager(QObject * parent) :
  QObject(parent)
{
  for (int i = 0; i < 2; i++) {
    LoopAndJumpPlayerData * data = new LoopAndJumpPlayerData;
    mPlayerData.push_back(data);
  }
}

void LoopAndJumpManager::playerTrigger(int player, QString name) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  if (name == "jump")
    playerSetValueInt(player, "jump", mPlayerData[player]->jump_next);
  else if (name == "jump_clear_next")
    mPlayerData[player]->clear_next = true;
  else if (name == "jump_clear_next_abort")
    mPlayerData[player]->clear_next = false;
}

void LoopAndJumpManager::playerSetValueInt(int player, QString name, int v) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  LoopAndJumpPlayerData * pdata = mPlayerData[player];
  
  if (name == "loading_work") {
    pdata->beats = nullptr;
    pdata->data.clear();
    pdata->work_id = v;
    pdata->jump_next = 0;
    emit(entriesCleared(player));
  } else if (name == "position_frame") {
    pdata->frame = v;
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
        emit(playerValueChangedInt(player, "seek_frame", it->frame));
      } else {
        int frame = pdata->frame;
        //find the closest beat
        //XXX make snap to be configurable!
        if (pdata->beats && pdata->beats->size() > 2) {
          if (frame <= pdata->beats->at(0)) {
            frame = pdata->beats->at(0);
          } else if (frame >= pdata->beats->back()) {
            frame = pdata->beats->back();
          } else {
            for (unsigned int i = 1; i < pdata->beats->size(); i++) {
              const int start = pdata->beats->at(i - 1);
              const int end = pdata->beats->at(i);
              if (frame >= start && frame < end) {
                //closer to which side?
                if (abs(frame - start) < abs(end - frame))
                  frame = start;
                else
                  frame = end;
                break;
              }
            }
          }
        }
        JumpOrLoopData data;
        data.frame = frame;
        pdata->data[v] = data;
        emit(entryUpdated(player, dj::loop_and_jump_type_t::JUMP, v, frame, frame));
      }
    }
  }
}

void LoopAndJumpManager::playerLoad(int player, djaudio::AudioBufferPtr /* audio_buffer */, djaudio::BeatBufferPtr beat_buffer) {
  if (player >= mPlayerData.size() || player < 0)
    return;
  mPlayerData[player]->beats = beat_buffer;
  mPlayerData[player]->data.clear();
  mPlayerData[player]->frame = 0;
  mPlayerData[player]->jump_next = 0;
  mPlayerData[player]->clear_next = false;
  emit(entriesCleared(player));
}

void LoopAndJumpManager::clearEntry(int player, int entry_index) {
  LoopAndJumpPlayerData * pdata = mPlayerData[player];
  auto it = pdata->data.find(entry_index);
  if (it != pdata->data.end())
    pdata->data.erase(it);
  emit(entryCleared(player, entry_index));
}

