#include "midimapper.hpp"

using namespace dj::controller;

MIDIMapper::MIDIMapper(QObject * parent): QThread(parent), mMappingState(IDLE), mAbort(false) {
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
      uint32_t key = ((uint32_t)buff.data[0] << 8) | buff.data[1];

      if (mMappingState == IDLE) {
         mapping_hash_t::iterator it = mMappings.find(key);
         if (it != mMappings.end()) {
            uint8_t input_value = buff.data[2];
            double value = static_cast<int>(it->value_offset + it->value_mul * (double)input_value / 127.0);
            switch(it->value_type) {
               case mapping_t::TRIGGER_VAL:
                  if (it->player_index >= 0)
                     emit(player_triggered(it->player_index, it->signal_name));
                  else
                     emit(master_triggered(it->signal_name));
                  break;
               case mapping_t::BOOL_VAL:
                  if (it->player_index >= 0)
                     emit(player_value_changed(it->player_index, it->signal_name, input_value != 0));
                  else
                     emit(master_value_changed(it->signal_name, input_value != 0));
                  break;
               case mapping_t::INT_VAL:
                  if (it->player_index >= 0)
                     emit(player_value_changed(it->player_index, it->signal_name, (int)value));
                  else
                     emit(master_value_changed(it->signal_name, (int)value));
                  break;
               case mapping_t::DOUBLE_VAL:
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
      }
   }
}

void MIDIMapper::map() {
   mMappingState = WAITING_SLOT;
}

void MIDIMapper::player_trigger(int player_index, QString name) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, mapping_t::TRIGGER_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, bool /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, mapping_t::BOOL_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, int /* value */) {
   if (name == "frame" || name == "speed") //TODO how to deal with speed?
      return;
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, mapping_t::INT_VAL);
}

void MIDIMapper::player_set(int player_index, QString name, double /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(player_index, name, mapping_t::DOUBLE_VAL);
}

void MIDIMapper::master_trigger(QString name) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, mapping_t::TRIGGER_VAL);
}

void MIDIMapper::master_set(QString name, bool /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, mapping_t::BOOL_VAL);
}

void MIDIMapper::master_set(QString name, int /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, mapping_t::INT_VAL);
}

void MIDIMapper::master_set(QString name, double /* value */) {
   if (mMappingState == WAITING_SLOT)
      mapping_from_slot(-1, name, mapping_t::DOUBLE_VAL);
}

void MIDIMapper::mapping_from_slot(int player_index, QString name, mapping_t::signal_val_t type) {
   mNextMapping.signal_name = name;
   mNextMapping.player_index = player_index;
   mNextMapping.value_type = type;
   mMappingState = WAITING_MIDI;
}
