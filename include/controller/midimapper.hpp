#ifndef DATAJOCKEY_MIDIMAPPER_HPP
#define DATAJOCKEY_MIDIMAPPER_HPP

#include "audioio.hpp"
#include "defines.hpp"
#include <QThread>
#include <QHash>

namespace dj {
   namespace controller {
      class MIDIMapper : public QThread {
         Q_OBJECT
         public:
            MIDIMapper(QObject * parent = NULL);
            virtual ~MIDIMapper();
            virtual void run();

            static const double value_multiplier_default;
            static const double value_offset_default;

            enum signal_val_t {
               TRIGGER_VAL,
               BOOL_VAL,
               INT_VAL,
               DOUBLE_VAL,
            };

         public slots:
            void map();
            void clear();
            void map_player(
                  int player_index,
                  QString signal_name,
                  int midi_status, int midi_param,
                  signal_val_t value_type,
                  double value_multiplier = value_multiplier_default,
                  double value_offset = value_offset_default);
            void map_master(
                  QString signal_name,
                  int midi_status, int midi_param,
                  signal_val_t value_type,
                  double value_multiplier = value_multiplier_default,
                  double value_offset = value_offset_default); 
            void load_file(QString file_path);
            void save_file(QString file_path);
            void auto_save(QString file_path);
            void auto_save(bool save);

            void player_trigger(int player_index, QString name);
            void player_set(int player_index, QString name, bool value);
            void player_set(int player_index, QString name, int value);
            void player_set(int player_index, QString name, double value);

            void master_trigger(QString name);
            void master_set(QString name, bool value);
            void master_set(QString name, int value);
            void master_set(QString name, double value);

         signals:
            void player_value_changed(int player_index, QString name, bool value);
            void player_value_changed(int player_index, QString name, int value);
            void player_value_changed(int player_index, QString name, double value);
            void player_triggered(int player_index, QString name);

            void master_value_changed(QString name, bool value);
            void master_value_changed(QString name, int value);
            void master_value_changed(QString name, double value);
            void master_triggered(QString name);

         private:
            enum map_state_t {
               IDLE,
               WAITING_SLOT,
               WAITING_MIDI
            };
            map_state_t mMappingState;

            audio::AudioIO::midi_ringbuff_t * mInputRingBuffer;
            bool mAbort;
            bool mAutoSave;
            QString mAutoSaveFileName;

            struct mapping_t {
               QString signal_name;
               signal_val_t value_type;
               int player_index; //if < 0 it is a master command

               //used to remap values
               double value_offset;
               double value_mul;

               void default_remaps() {
                  value_offset = 0;
                  value_mul = (double)one_scale;
               }

               mapping_t() {
                  default_remaps();
               }
            };

            //(status byte << 8) || num -> mapping
            typedef QHash<uint32_t, mapping_t> mapping_hash_t;
            mapping_hash_t mMappings;
            mapping_t mNextMapping;

            void mapping_from_slot(int player_index, QString name, signal_val_t type);
      };
   }
}

#endif
