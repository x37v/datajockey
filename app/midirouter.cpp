#include "midirouter.h"
#include "jackmidiport.hpp"
#include "defines.hpp"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QTextStream>
#include <map>
#include <stdexcept>

namespace {
  typedef QSharedPointer<MidiMap> MidiMapPtr;

  QList<QString> player_triggers = {
    "cue", "play", "sync", "load", "seek_fwd", "seek_back", "jump", "jump_clear_next", "jump_new"
  };

  QList<QString> player_bool = {
     "sync", "cue", "bump_fwd", "bump_back",
  };

  QList<QString> player_continuous = {
    "volume", "eq_high", "eq_mid", "eq_low"
  };

  QList<QString> master_triggers = {
    "sync_to_player0", "sync_to_player1"
  };

  QList<QString> master_continuous = {
    "volume", "bpm"
  };

  bool double_signal(QString name) {
    return name.contains("bpm") || name.contains("speed");
  }

  enum midi_t {
    NOTE,
    NOTE_ON,
    NOTE_OFF,
    CC
  };

  enum mapping_t {
    TRIGGER,
    BOOL,
    CONTINUOUS,
    TWOS_COMPLEMENT,
    SHIFT
  };

  const std::map<QString, midi_t> yamls_trigger_map = {
    {"note_on", NOTE_ON},
    {"note_off", NOTE_OFF},
    {"cc", CC}
  };

  const std::map<QString, midi_t> yamls_cc_map = {
    {"note", NOTE},
    {"note_on", NOTE_ON},
    {"note_off", NOTE_OFF},
    {"cc", CC}
  };


}

class MidiMap {
  public:
    bool matches(djaudio::AudioIO::midi_event_buffer_t event);
    double offset = 0.0;
    double multiplier = 1.0;
    mapping_t mapping_type = BOOL;
    midi_t midi_type = NOTE;
    int channel = 0;
    int number = 0;; //note or cc #

    int player_index = -1; //< 0, non player
    QString signal_name;
};

namespace {
  template <typename T>
    bool find_midi(YAML::Node& node, MidiMapPtr& mmap, T map) throw (std::runtime_error) {
      //find the trigger type and number/channel
      for (auto kv: map) {
        if (node[kv.first]) {
          mmap->midi_type = kv.second;
          YAML::Node midinode = node[kv.first];
          if (!midinode.IsSequence())
            throw std::runtime_error(kv.first.toStdString() + " mapping is not a sequence");
          //example note_off: [96, 3]
          mmap->number = midinode[0].as<int>();
          mmap->channel = midinode[1].as<int>();
          return true;
        }
      }
      return false;
    }
}

MidiRouter::MidiRouter(djaudio::AudioIO::midi_ringbuff_t *ringbuf, QObject *parent) :
  QObject(parent),
  mInputRingBuffer(ringbuf)
{
}

void MidiRouter::clear() {
  mMappings.clear();
}

void MidiRouter::readFile(QString fileName) {
  clear();

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    emit(mappingError("cannot open file: " + fileName));
    return;
  }
  QTextStream in(&file);
  YAML::Node root = YAML::Load(in.readAll().toStdString());
  if (!root.IsMap()) {
    emit(mappingError("mapping is not a map in " + fileName));
    return;
  }

  for (auto kv: std::map<QString, int>({{"master", -1}, {"player0", 0}, {"player1", 1}})) {
    const QString key = kv.first;
    const int player = kv.second;

    if (!root[key])
      continue;
    YAML::Node parent = root[key];
    if (!parent.IsSequence())
      emit(mappingError(key + " is not a sequence in " + fileName));

    for (unsigned int i = 0; i < parent.size(); i++) {
      YAML::Node node = parent[i];
      MidiMapPtr mmap = QSharedPointer<MidiMap>::create();
      mmap->player_index = player;
      try {
        if (node["trigger"]) {
          mmap->signal_name = node["trigger"].as<QString>();
          mmap->mapping_type = TRIGGER;
          try {
            if (!find_midi(node, mmap, yamls_trigger_map))
              emit(mappingError(key + " could not find trigger mapping in element " + QString::number(i)));
          } catch (std::runtime_error& e) {
            emit(mappingError(key + QString::fromStdString(e.what()) + " in element " + QString::number(i)));
          }
        } else if (node["bool"]) {
          mmap->signal_name = node["bool"].as<QString>();
          mmap->mapping_type = BOOL;
          try {
            if (!find_midi(node, mmap, yamls_cc_map))
              emit(mappingError(key + " could not find bool mapping in element " + QString::number(i)));
          } catch (std::runtime_error& e) {
            emit(mappingError(key + QString::fromStdString(e.what()) + " in element " + QString::number(i)));
          }
        } else if (node["continuous"]) {
          mmap->signal_name = node["continuous"].as<QString>();
          mmap->mapping_type = CONTINUOUS;
          try {
            if (!find_midi(node, mmap, yamls_cc_map))
              emit(mappingError(key + " could not find cc mapping in element " + QString::number(i)));
          } catch (std::runtime_error& e) {
            emit(mappingError(key + QString::fromStdString(e.what()) + " in element " + QString::number(i)));
          }
        } else if (node["twos_complement"]) {
          mmap->signal_name = node["twos_complement"].as<QString>();
          mmap->mapping_type = TWOS_COMPLEMENT;
          try {
            if (!find_midi(node, mmap, yamls_cc_map))
              emit(mappingError(key + " could not find trigger mapping in element " + QString::number(i)));
          } catch (std::runtime_error& e) {
            emit(mappingError(key + QString::fromStdString(e.what()) + " in element " + QString::number(i)));
          }
        } else {
          emit(mappingError(key + " no supported mapping found in midi mapping element " + QString::number(i)));
          return;
        }

        if (node["mult"])
          mmap->multiplier = node["mult"].as<double>();
        if (node["offset"])
          mmap->offset = node["offset"].as<double>();

        //XXX search for conflicting maps and warn
        mMappings.push_back(mmap);
      } catch (YAML::Exception& e) {
        emit(mappingError(key + " exception processing midi mapping element " + QString::number(i) + " " + QString(e.what())));
        return;
      } catch(...) {
        emit(mappingError(key + " exception processing midi mapping element " + QString::number(i)));
        return;
      }
    }
  }
}

#include <iostream>
using namespace std;

void MidiRouter::process() {
  while (mInputRingBuffer->getReadSpace()) {
    djaudio::AudioIO::midi_event_buffer_t buff;
    mInputRingBuffer->read(buff);

    for (auto mmap: mMappings) {
      if (mmap->matches(buff)) {
        double value = mmap->offset + mmap->multiplier * static_cast<double>(buff.data[2]) / 127.0;
        int intvalue = value * static_cast<double>(dj::one_scale);
        int player = mmap->player_index;
        QString signal_name = mmap->signal_name;
        const uint8_t status = buff.data[0] & 0xF0;
        switch (mmap->mapping_type) {
          case TRIGGER:
            //only non zero CCs trigger
            if (mmap->midi_type == CC && value <= 0)
              continue;
            if (player < 0)
              emit(masterTriggered(signal_name));
            else
              emit(playerTriggered(player, signal_name));
            break;
          case TWOS_COMPLEMENT:
            //if the top bit is set it is negative, we don't scale 2s complement by 127
            value = mmap->offset + mmap->multiplier * static_cast<double>((buff.data[2] & 0x40) ? (buff.data[2] - 128) : buff.data[2]);
            intvalue = value * static_cast<double>(dj::one_scale);
            if (signal_name.contains("select_work"))
              intvalue = value;
            //intentional fall through
          case CONTINUOUS:
            if (double_signal(signal_name)) {
              if (player < 0)
                emit(masterValueChangedDouble(signal_name, value));
              else
                emit(playerValueChangedDouble(player, signal_name, value));
            } else {
              if (player < 0)
                emit(masterValueChangedInt(signal_name, intvalue));
              else
                emit(playerValueChangedInt(player, signal_name, intvalue));
            }
            break;
          case BOOL:
            if (player >= 0)
              emit (playerValueChangedBool(player, signal_name, buff.data[2] > 0 && status != JackCpp::MIDIPort::NOTEOFF));
            break;
          case SHIFT:
            break;
        }
      }
    }
  }
}

bool MidiMap::matches(djaudio::AudioIO::midi_event_buffer_t event) {
  //make sure channel and number are correct
  if ((event.data[0] & 0x0F) != channel)
    return false;
  if (event.data[1] != number)
    return false;

  //check the type of message
  uint8_t status = event.data[0] & 0xF0;
  switch (midi_type) {
    case NOTE:
      if (status == JackCpp::MIDIPort::NOTEON || status == JackCpp::MIDIPort::NOTEOFF)
        return true;
      break;
    case NOTE_ON:
      if (status == JackCpp::MIDIPort::NOTEON)
        return true;
      break;
    case NOTE_OFF:
      if (status == JackCpp::MIDIPort::NOTEOFF)
        return true;
      break;
    case CC:
      if (status == JackCpp::MIDIPort::CC)
        return true;
      break;
  }
  return false;
}

