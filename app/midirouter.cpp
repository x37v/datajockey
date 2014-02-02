#include "midirouter.h"
#include "jackmidiport.hpp"
#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QTextStream>

namespace {
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
    INTEGER,
    SHIFT
  };
}

class MidiMap {
  public:
    bool matches(jack_midi_event_t event);
    double offset = 0.0;
    double multiplier = 1.0;
};

MidiRouter::MidiRouter(QObject *parent) :
  QObject(parent)
{
}

void MidiRouter::clear() {
  mMappings.clear();
}

void MidiRouter::readFile(QString fileName) {
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return; //XXX error
  QTextStream in(&file);
  YAML::Node root = YAML::Load(in.readAll().toStdString());
}


bool MidiMap::matches(jack_midi_event_t event) {
  return false;
}
