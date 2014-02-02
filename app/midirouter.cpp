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
    "cue", "play", "sync", "load", "seek_fwd", "seek_back", "bump_fwd", "bump_back"
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
    SHIFT
  };

  const std::map<QString, midi_t> yamls_trigger_map = {
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
  if (!root.IsSequence()) {
    emit(mappingError("mapping is not a sequence in " + fileName));
    return;
  }
  for (unsigned int i = 0; i < root.size(); i++) {
    YAML::Node node = root[i];
    MidiMapPtr mmap = QSharedPointer<MidiMap>::create();
    try {
      if (node["player"])
        mmap->player_index = node["player"].as<int>();
      if (node["trigger"]) {
        mmap->signal_name = node["trigger"].as<QString>();
        mmap->mapping_type = TRIGGER;
        try {
          if (!find_midi(node, mmap, yamls_trigger_map))
            emit(mappingError("could not find trigger mapping in " + fileName));
        } catch (std::runtime_error& e) {
          emit(mappingError(QString::fromStdString(e.what()) + fileName));
        }
      } else if (node["continuous"]) {
        mmap->signal_name = node["continuous"].as<QString>();
        mmap->mapping_type = CONTINUOUS;
        mmap->midi_type = CC;
        if (!(node["cc"] && node["cc"].IsSequence())) {
          emit(mappingError("no cc sequence found in midi mapping element " + QString::number(i)));
          return;
        }
        mmap->number = node["cc"][0].as<int>();
        mmap->channel = node["cc"][1].as<int>();
      } else {
        emit(mappingError("no supported mapping found in midi mapping element " + QString::number(i)));
        return;
      }

      if (node["mult"])
        mmap->multiplier = node["mult"].as<double>();
      if (node["offset"])
        mmap->offset = node["offset"].as<double>();

      //XXX search for conflicting maps and warn
      mMappings.push_back(mmap);
    } catch (YAML::Exception& e) {
      emit(mappingError("exception processing midi mapping element " + QString::number(i) + " " + QString(e.what())));
      return;
    } catch(...) {
      emit(mappingError("exception processing midi mapping element " + QString::number(i)));
      return;
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
          case CONTINUOUS:
            //check to see if we should actually emit a double
            if (player < 0)
              emit(masterValueChangedInt(signal_name, intvalue));
            else
              emit(playerValueChangedInt(player, signal_name, intvalue));
            break;
          case BOOL:
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

