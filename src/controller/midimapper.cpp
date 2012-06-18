#include "midimapper.hpp"
#include <yaml-cpp/yaml.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <fstream>
#include <stdexcept>
#include <iostream>

using namespace dj::controller;
using std::cerr;
using std::endl;

const double MIDIMapper::value_multiplier_default = (double)one_scale;
const double MIDIMapper::value_offset_default = 0.0;

uint32_t make_key(uint8_t status, uint8_t param) {
  return (status << 8) | param;
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
      uint32_t key = make_key(buff.data[0], buff.data[1]);

      if (mMappingState == IDLE) {
         mapping_hash_t::iterator it = mMappings.find(key);
         if (it != mMappings.end()) {
            uint8_t input_value = buff.data[2];
            double value = static_cast<int>(it->value_offset + it->value_mul * (double)input_value / 127.0);
            switch(it->value_type) {
               case TRIGGER_VAL:
                  if (it->player_index >= 0)
                     emit(player_triggered(it->player_index, it->signal_name));
                  else
                     emit(master_triggered(it->signal_name));
                  break;
               case BOOL_VAL:
                  if (it->player_index >= 0)
                     emit(player_value_changed(it->player_index, it->signal_name, input_value != 0));
                  else
                     emit(master_value_changed(it->signal_name, input_value != 0));
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
         }
      } else if (mMappingState == WAITING_MIDI) {

         //set up value remapings
         mNextMapping.default_remaps();
         if (mNextMapping.signal_name == "bpm") {
            mNextMapping.value_offset = 40;
            mNextMapping.value_mul = 160;
         } else if (mNextMapping.signal_name == "volume") {
            mNextMapping.value_mul = 1.5 * (double)one_scale;
         } else if (mNextMapping.signal_name.contains("eq_")) {
            mNextMapping.value_mul = 2.0 * (double)one_scale;
            mNextMapping.value_offset = -(double)one_scale;
         }

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
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      int midi_status, int midi_param,
      signal_val_t value_type,
      double value_multiplier,
      double value_offset) { 
   if (player_index < 0) {
      map_master(signal_name, midi_status, midi_param, value_type, value_multiplier, value_offset);
      return;
   }
   mapping_t mapping;
   mapping.signal_name = signal_name;
   mapping.player_index = player_index;
   mapping.value_type = value_type;
   mapping.value_offset = value_offset;
   mapping.value_mul = value_multiplier;
   uint32_t key = make_key(0xFF & midi_status, 0xFF & midi_param);
   mMappings.insert(key, mapping);
}

void MIDIMapper::map_master(
      QString signal_name,
      int midi_status, int midi_param,
      signal_val_t value_type,
      double value_multiplier,
      double value_offset) {
   mapping_t mapping;
   mapping.signal_name = signal_name;
   mapping.player_index = -1;
   mapping.value_type = value_type;
   mapping.value_offset = value_offset;
   mapping.value_mul = value_multiplier;
   uint32_t key = make_key(0xFF & midi_status, 0xFF & midi_param);
   mMappings.insert(key, mapping);
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
            double value_offset = value_offset_default;
            double value_multiplier = value_multiplier_default;

            int midi_status; entry["midi_status"] >> midi_status;
            int midi_param; entry["midi_param"] >> midi_param;
            std::string signal_name; entry["signal_name"] >> signal_name;
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

            //optional values
            if ((find_value = entry.FindValue("value_offset")))
               *find_value >> value_offset;
            if ((find_value = entry.FindValue("value_multiplier")))
               *find_value >> value_multiplier;


            if ((find_value = entry.FindValue("player_index"))) {
               int player_index;
               *find_value >> player_index;
               map_player(
                     player_index,
                     QString::fromStdString(signal_name),
                     midi_status, midi_param,
                     value_type, value_multiplier, value_offset);
            } else {
               map_master(
                     QString::fromStdString(signal_name),
                     midi_status, midi_param,
                     value_type, value_multiplier, value_offset);
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
   cerr << "saving to file " + file_path.toStdString() << endl;
   if (mMappings.empty())
      return;

   YAML::Emitter yaml;
   yaml << YAML::BeginSeq;
   for(mapping_hash_t::const_iterator it = mMappings.constBegin(); it != mMappings.constEnd(); it++) {
      yaml << YAML::BeginMap;
      yaml << YAML::Key << "midi_status" << YAML::Value << (it.key() >> 8);
      yaml << YAML::Key << "midi_param" << YAML::Value << (it.key() & 0xFF);
      yaml << YAML::Key << "signal_name" << YAML::Value << it.value().signal_name.toStdString();
      yaml << YAML::Key << "value_type";
      switch (it.value().value_type) {
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
      yaml << YAML::Key << "value_offset" << YAML::Value << it.value().value_offset;
      yaml << YAML::Key << "value_multiplier" << YAML::Value << it.value().value_mul;

      int player_index = it.value().player_index;
      if (player_index >= 0)
         yaml << YAML::Key << "player_index" << YAML::Value << player_index;

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
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, TRIGGER_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, bool /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, BOOL_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, int /* value */) {
   if (name == "frame" || name == "speed") //TODO how to deal with speed?
      return;
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, INT_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, double /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, DOUBLE_VAL);
}

void MIDIMapper::master_trigger(QString name) {
   if (mMappingState == WAITING_SLOT) mapping_from_slot(-1, name, TRIGGER_VAL);
}

void MIDIMapper::master_set(QString name, bool /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, BOOL_VAL);
}

void MIDIMapper::master_set(QString name, int /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, INT_VAL);
}

void MIDIMapper::master_set(QString name, double /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, DOUBLE_VAL);
}

void MIDIMapper::mapping_from_slot(int player_index, QString name, signal_val_t type) {
   mNextMapping.signal_name = name;
   mNextMapping.player_index = player_index;
   mNextMapping.value_type = type;
   mMappingState = WAITING_MIDI;
}
