#include "audiomodel.hpp"
#include "midimapper.hpp"
#include <yaml-cpp/yaml.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include "jackmidiport.hpp"

using namespace dj::controller;
using dj::audio::AudioModel;

using std::cerr;
using std::endl;

namespace {
   midimapping_t status_to_mapping_t(uint8_t status) {
      switch (status & 0xF0) {
         case JackCpp::MIDIPort::NOTEON:
            return NOTE_ON;
         case JackCpp::MIDIPort::NOTEOFF:
            return NOTE_TOGGLE;
         default:
            return CONTROL_CHANGE;
      }
   }

   uint32_t make_key(midimapping_t midi_type, uint8_t channel, uint8_t param) {
      uint8_t status = JackCpp::MIDIPort::CC;
      if (midi_type == NOTE_ON)
         status = JackCpp::MIDIPort::NOTEON;
      else if (midi_type == NOTE_TOGGLE) //
         status = JackCpp::MIDIPort::NOTEOFF;

      return ((status | (channel & 0xF)) << 8) | param;
   }

   void key_info(uint32_t key, midimapping_t& midi_type, uint8_t& channel, uint8_t& param) {
      midi_type = status_to_mapping_t((key >> 8) & 0xF0);
      channel = (key >> 8) & 0xF;
      param = key & 0xFF;
   }

}

void MIDIMapper::default_value_mappings(const QString& signal_name, double& offset, double& mult) {
   mult = 1.0;
   offset = 0.0;

   if (signal_name.contains("relative")) { //XXX what about beats or frames?
      mult = 0.1;
   } else if (signal_name == "bpm") {
      offset = 40.0;
      mult = 160.0;
   } else if (signal_name == "volume") {
      mult = 1.5;
   } else if (signal_name.contains("eq_") || signal_name.contains("speed")) {
      mult = 2.0;
      offset = -1.0;
   }
}

MIDIMapper::MIDIMapper(QObject * parent): QThread(parent), mMappingState(IDLE), mAbort(false), mAutoSave(false) {
   mInputRingBuffer = audio::AudioIO::instance()->midi_input_ringbuffer();
}

MIDIMapper::~MIDIMapper() {
}

void MIDIMapper::run() {
   setPriority(QThread::HighPriority);

   while (!mAbort) {
      audio::AudioIO::midi_event_buffer_t buff;
      if (!mInputRingBuffer->getReadSpace()) {
         QThread::msleep(5);
         continue;
      }
      mInputRingBuffer->read(buff);

      //look up this event
      uint32_t key = make_key(status_to_mapping_t(buff.data[0] & 0xF0), buff.data[0] & 0x0F, buff.data[1]);

      if (mMappingState == IDLE) {
         mapping_hash_t::iterator it = mMappings.find(key);
         if (it != mMappings.end()) {
            uint8_t input_value = buff.data[2];
            double value = static_cast<int>(it->value_offset + it->value_mul * (double)input_value / 127.0);

            //multiply by one_scale when we should
            if (it->signal_name.contains("eq") ||
                  it->signal_name.contains("speed") ||
                  it->signal_name.contains("volume") ||
                  it->signal_name.contains("crossfade_position"))
               value *= one_scale;

            switch(it->value_type) {
               case TRIGGER_VAL:
                  //trigger when we cross the 0.5 mark
                  if (it->value_last < 0.5 && value > 0.5) {
                     if (it->player_index >= 0)
                        emit(player_triggered(it->player_index, it->signal_name));
                     else
                        emit(master_triggered(it->signal_name));
                  }
                  break;
               case BOOL_VAL:
                  //toggle when we cross the 0.5 boundary
                  if (it->value_last < 0.5 && value > 0.5) {
                     if (it->player_index >= 0)
                        emit(player_value_changed(it->player_index, it->signal_name, true));
                     else
                        emit(master_value_changed(it->signal_name, true));
                  } else if (it->value_last > 0.5 && value < 0.5) {
                     if (it->player_index >= 0)
                        emit(player_value_changed(it->player_index, it->signal_name, false));
                     else
                        emit(master_value_changed(it->signal_name, false));
                  }
                  break;
               case INT_VAL:
                  if (it->player_index >= 0)
                     emit(player_value_changed(it->player_index, it->signal_name, (int)value));
                  else
                     emit(master_value_changed(it->signal_name, (int)value));
                  break;
               case DOUBLE_VAL:
                  if (it->player_index >= 0)
                     emit(player_value_changed(it->player_index, it->signal_name, value));
                  else
                     emit(master_value_changed(it->signal_name, value));
                  break;
            }
            it->value_last = value;
         }
      } else if (mMappingState == WAITING_MIDI) {

         //set up value remapings
         default_value_mappings(mNextMapping.signal_name, mNextMapping.value_offset, mNextMapping.value_mul);

         mMappings.insert(key, mNextMapping);
         mMappingState = IDLE;

         //auto save if enabled
         if (mAutoSave && !mAutoSaveFileName.isEmpty())
            save_file(mAutoSaveFileName);
      }
   }
}

void MIDIMapper::map() {
   mMappingState = WAITING_SLOT;
}

void MIDIMapper::clear() {
   mMappingState = IDLE;
   mMappings.clear();
   emit(mappings_cleared());
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      midimapping_t midi_type,
      int midi_channel,
      int midi_param) {
   double value_multiplier, value_offset;
   default_value_mappings(signal_name, value_offset, value_multiplier);
   MIDIMapper::map_player(player_index, signal_name, midi_type, midi_channel, midi_param, value_multiplier, value_offset);
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      midimapping_t midi_type,
      int midi_channel,
      int midi_param,
      double value_multiplier,
      double value_offset) { 
   if (player_index < 0) {
      map_master(signal_name, midi_type, midi_channel, midi_param, value_multiplier, value_offset);
      return;
   }
   mapping_t mapping;
   mapping.midi_type = midi_type;
   mapping.signal_name = signal_name;
   mapping.player_index = player_index;
   mapping.value_type = player_value_type(signal_name);
   mapping.value_offset = value_offset;
   mapping.value_mul = value_multiplier;

   uint32_t key = make_key(midi_type, midi_channel, 0xFF & midi_param);
   mMappings.insert(key, mapping);
   emit(player_mapping_update(player_index, signal_name, midi_type, midi_channel, midi_param, value_multiplier, value_offset));
}

void MIDIMapper::map_master(
      QString signal_name,
      midimapping_t midi_type,
      int midi_channel,
      int midi_param) {
   double value_multiplier, value_offset;
   default_value_mappings(signal_name, value_offset, value_multiplier);
   map_master(signal_name, midi_type, midi_channel, midi_param, value_multiplier, value_offset);
}

void MIDIMapper::map_master(
      QString signal_name,
      midimapping_t midi_type,
      int midi_channel,
      int midi_param,
      double value_multiplier,
      double value_offset) {
   mapping_t mapping;
   mapping.midi_type = midi_type;
   mapping.signal_name = signal_name;
   mapping.player_index = -1;
   mapping.value_type = master_value_type(signal_name);
   mapping.value_offset = value_offset;
   mapping.value_mul = value_multiplier;
   uint32_t key = make_key(midi_type, midi_channel, 0xFF & midi_param);
   mMappings.insert(key, mapping);
   emit(master_mapping_update(signal_name, midi_type, midi_channel, midi_param, value_multiplier, value_offset));
}

void MIDIMapper::load_file(QString file_path) {
   clear();
   try {
      if (!QFile::exists(file_path))
         throw(std::runtime_error("file does not exist: " + file_path.toStdString()));
      std::ifstream fin(file_path.toStdString().c_str());
      YAML::Parser parser(fin);
      YAML::Node doc;
      parser.GetNextDocument(doc);
      if (doc.Type() != YAML::NodeType::Sequence)
         throw(std::runtime_error("should be a sequence at the top level: " + file_path.toStdString()));
      for (unsigned int i = 0; i < doc.size(); i++) {
         try {
            const YAML::Node& entry = doc[i];
            const YAML::Node * find_value;
            double value_offset;
            double value_multiplier;

            std::string midi_type_name; entry["midi_type"] >> midi_type_name;
            midimapping_t midi_type;
            if (midi_type_name == "note_on")
               midi_type = NOTE_ON;
            else if (midi_type_name == "note_toggle")
               midi_type = NOTE_TOGGLE;
            else if (midi_type_name == "control_change")
               midi_type = CONTROL_CHANGE;
            else
               throw (std::runtime_error(DJ_FILEANDLINE + midi_type_name + " is not a supported midi type for mapping"));

            int midi_param; entry["midi_param"] >> midi_param;
            int midi_channel; entry["midi_channel"] >> midi_channel;
            std::string signal_name; entry["signal"] >> signal_name;

            /*
            std::string value_type_name; entry["value_type"] >> value_type_name;

            signal_val_t value_type;
            if (value_type_name == "trigger") {
               value_type = TRIGGER_VAL;
            } else if (value_type_name == "bool") {
               value_type = BOOL_VAL;
            } else if (value_type_name == "double") {
               value_type = DOUBLE_VAL;
            } else if (value_type_name == "int") {
               value_type = INT_VAL;
            } else
               throw(std::runtime_error("not a supported value type: " + value_type_name));
               */

            //set up offsets and mults
            default_value_mappings(QString::fromStdString(signal_name), value_offset, value_multiplier);
            if ((find_value = entry.FindValue("value_offset")))
               *find_value >> value_offset;
            if ((find_value = entry.FindValue("value_multiplier")))
               *find_value >> value_multiplier;


            if ((find_value = entry.FindValue("player"))) {
               int player_index;
               *find_value >> player_index;
               map_player(
                     player_index,
                     QString::fromStdString(signal_name),
                     midi_type, midi_channel, midi_param,
                     value_multiplier, value_offset);
            } else {
               map_master(
                     QString::fromStdString(signal_name),
                     midi_type, midi_channel, midi_param,
                     value_multiplier, value_offset);
            }
         } catch (std::runtime_error& e) {
            //TODO actually report errors
            cerr << "problem parsing file: " + file_path.toStdString() + " exception: " + e.what() << endl;
         } catch (...) {
            cerr << "problem parsing file: " + file_path.toStdString() << endl;
         }
      }
   } catch (std::runtime_error& e) {
      cerr << "problem parsing file: " + file_path.toStdString() + " exception: " + e.what() << endl;
   } catch (...) {
      cerr << "problem parsing file: " + file_path.toStdString() << endl;
   }
}

void MIDIMapper::save_file(QString file_path) {
   if (mMappings.empty())
      return;

   YAML::Emitter yaml;
   yaml << YAML::BeginSeq;
   for(mapping_hash_t::const_iterator it = mMappings.constBegin(); it != mMappings.constEnd(); it++) {
      const mapping_t& mapping = it.value();
      yaml << YAML::BeginMap;
      //TODO make configurable
      yaml << YAML::Key << "signal" << YAML::Value << mapping.signal_name.toStdString();
      int player_index = mapping.player_index;
      if (player_index >= 0)
         yaml << YAML::Key << "player" << YAML::Value << player_index;

      midimapping_t midi_type;
      uint8_t midi_channel, midi_param;
      key_info(it.key(), midi_type, midi_channel, midi_param);

      yaml << YAML::Key << "midi_type";
      switch(midi_type) {
         case NOTE_ON:
            yaml << YAML::Value << "note_on";
            break;
         case NOTE_TOGGLE:
            yaml << YAML::Value << "note_toggle";
            break;
         default:
            yaml << YAML::Value << "control_change";
            break;
      }
      yaml << YAML::Key << "midi_channel" << YAML::Value << (int)midi_channel;
      yaml << YAML::Key << "midi_param" << YAML::Value << (int)midi_param;

      /*
      yaml << YAML::Key << "value_type";
      switch (mapping.value_type) {
         case TRIGGER_VAL:
            yaml << YAML::Value << "trigger";
            break;
         case BOOL_VAL:
            yaml << YAML::Value << "bool";
            break;
         case DOUBLE_VAL:
            yaml << YAML::Value << "double";
            break;
         case INT_VAL:
            yaml << YAML::Value << "int";
            break;
      }
      */

      //only write non defaults
      double default_offset, default_mult;
      default_value_mappings(mapping.signal_name, default_offset, default_mult);
      if (mapping.value_offset != default_offset)
         yaml << YAML::Key << "value_offset" << YAML::Value << mapping.value_offset;
      if (mapping.value_mul != default_mult)
         yaml << YAML::Key << "value_multiplier" << YAML::Value << mapping.value_mul;

      yaml << YAML::EndMap;
   }
   yaml << YAML::EndSeq;

   //make sure the containing directory exists
   QFileInfo info(file_path);
   if (!info.dir().exists())
      info.dir().mkpath(info.dir().path());

   QFile file(file_path);
   if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
      throw(std::runtime_error(DJ_FILEANDLINE + " cannot open file: " + file_path.toStdString()));
   QTextStream out(&file);
   out << QString(yaml.c_str());
}

void MIDIMapper::auto_save(QString file_path) {
   mAutoSaveFileName = file_path;
   auto_save(true);
}

void MIDIMapper::auto_save(bool save) { mAutoSave = save; }

void MIDIMapper::player_trigger(int player_index, QString name) {
   mapping_from_slot(player_index, name, TRIGGER_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, bool /* value */) {
   //we default to toggles for boolean
   mapping_from_slot(player_index, name, TRIGGER_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, int /* value */) {
   mapping_from_slot(player_index, name, INT_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, double /* value */) {
   mapping_from_slot(player_index, name, DOUBLE_VAL);
}

void MIDIMapper::master_trigger(QString name) {
   mapping_from_slot(-1, name, TRIGGER_VAL);
}

void MIDIMapper::master_set(QString name, bool /* value */) {
   mapping_from_slot(-1, name, BOOL_VAL);
}

void MIDIMapper::master_set(QString name, int /* value */) {
   mapping_from_slot(-1, name, INT_VAL);
}

void MIDIMapper::master_set(QString name, double /* value */) {
   mapping_from_slot(-1, name, DOUBLE_VAL);
}

void MIDIMapper::mapping_from_slot(int player_index, QString name, signal_val_t type) {
   if (mMappingState != WAITING_SLOT || name.contains("update_"))
      return;
   mNextMapping.signal_name = name;
   mNextMapping.player_index = player_index;
   mNextMapping.value_type = type;
   mMappingState = WAITING_MIDI;
}

MIDIMapper::signal_val_t MIDIMapper::player_value_type(QString signal_name) {
   if (signal_name.contains("relative") || AudioModel::player_signals["trigger"].contains(signal_name) || AudioModel::player_signals["bool"].contains(signal_name))
      return TRIGGER_VAL;
   if (AudioModel::player_signals["int"].contains(signal_name))
      return INT_VAL;
   if (AudioModel::player_signals["double"].contains(signal_name))
      return DOUBLE_VAL;
   return TRIGGER_VAL;
}


MIDIMapper::signal_val_t MIDIMapper::master_value_type(QString signal_name) {
   if (signal_name.contains("relative") || AudioModel::master_signals["trigger"].contains(signal_name) || AudioModel::master_signals["bool"].contains(signal_name))
      return TRIGGER_VAL;
   if (AudioModel::master_signals["int"].contains(signal_name))
      return INT_VAL;
   if (AudioModel::master_signals["double"].contains(signal_name))
      return DOUBLE_VAL;
   return TRIGGER_VAL;
}

