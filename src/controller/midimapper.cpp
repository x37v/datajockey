#include "midimapper.hpp"

using namespace dj::controller;

MIDIMapper::MIDIMapper(QObject * parent): QThread(parent), mAbort(false) {
   mInputRingBuffer = audio::AudioIO::instance()->midi_input_ringbuffer();

   //XXX just to try out
   mapping_t mapping;
   uint32_t key = (0xB5 << 8) | 40;
   mapping.player_index = 0;
   mapping.signal_name = "volume";
   mapping.value_type = mapping_t::INT_VAL;
   mapping.value_mul = (double)dj::one_scale * 1.5;
   mMappings.insert(key, mapping);

   key = (0xB5 << 8) | 41;
   mapping.player_index = 1;
   mMappings.insert(key, mapping);
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
                  emit(player_toggled(it->player_index, it->signal_name, input_value != 0));
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
   }
}

void MIDIMapper::player_trigger(int player_index, QString name) {
}

void MIDIMapper::player_set(int player_index, QString name, bool value) {
}

void MIDIMapper::player_set(int player_index, QString name, int value) {
}

void MIDIMapper::player_set(int player_index, QString name, double value) {
}

void MIDIMapper::master_set(QString name, bool value) {
}

void MIDIMapper::master_set(QString name, int value) {
}

void MIDIMapper::master_set(QString name, double value) {
}

