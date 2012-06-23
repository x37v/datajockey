#ifndef DATAJOCKEY_MIDIMAPPER_HPP
#define DATAJOCKEY_MIDIMAPPER_HPP

#include "audioio.hpp"
#include "defines.hpp"
#include <QThread>
#include <QHash>
#include <QString>

namespace dj {
   namespace controller {
      enum midimapping_t {
         NO_MAPPING,
         CONTROL_CHANGE, //crossing 0.5 in the positive triggers/toggles or turns on, crossing 0.5 in the negative turns off
         NOTE_ON, //note on triggers/toggles
         NOTE_TOGGLE //note on turns on, note off turns off
      };

      class MIDIMapper : public QThread {
         Q_OBJECT
         public:
            MIDIMapper(QObject * parent = NULL);
            virtual ~MIDIMapper();
            virtual void run();

            enum signal_val_t {
               TRIGGER_VAL,
               BOOL_VAL,
               INT_VAL,
               DOUBLE_VAL,
            };

            static void default_value_mappings(const QString& signal_name, double& offset, double& mult);

         public slots:
            void map();
            void clear();

            void map_player(
                  int player_index,
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param);
            void map_player(
                  int player_index,
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);

            void map_master(
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param);
            void map_master(
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);

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
            void mappings_cleared();
            void player_mapping_update(int player_index, QString signal_name, 
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);
            void master_mapping_update(QString signal_name, 
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);

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

               midimapping_t midi_type;
               signal_val_t value_type;

               int player_index; //if < 0 it is a master command

               //used to remap values
               double value_offset;
               double value_mul;
               double value_last;

               mapping_t() : value_last(-1.0) {
               }
            };

            //(status byte << 8) || num -> mapping
            typedef QHash<uint32_t, mapping_t> mapping_hash_t;
            mapping_hash_t mMappings;
            mapping_t mNextMapping;

            void mapping_from_slot(int player_index, QString name, signal_val_t type);

            signal_val_t player_value_type(QString signal_name);
            signal_val_t master_value_type(QString signal_name);
      };
   }
}

#endif
