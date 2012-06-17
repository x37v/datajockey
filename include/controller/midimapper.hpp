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
         public slots:
            void player_trigger(int player_index, QString name);
            void player_set(int player_index, QString name, bool value);
            void player_set(int player_index, QString name, int value);
            void player_set(int player_index, QString name, double value);

            void master_set(QString name, bool value);
            void master_set(QString name, int value);
            void master_set(QString name, double value);

         signals:
            void player_value_changed(int player_index, QString name, int value);
            void player_toggled(int player_index, QString name, bool value);
            void player_triggered(int player_index, QString name);

            void master_value_changed(QString name, bool value);
            void master_value_changed(QString name, int value);
            void master_value_changed(QString name, double value);

         private:
            audio::AudioIO::midi_ringbuff_t * mInputRingBuffer;
            bool mAbort;
            struct mapping_t {
               enum signal_val_t {
                  TRIGGER_VAL,
                  BOOL_VAL,
                  INT_VAL,
                  DOUBLE_VAL,
               };
               QString signal_name;
               signal_val_t value_type;
               int player_index; //if < 0 it is a master command

               //used to remap values
               double value_offset;
               double value_mul;

               mapping_t() : value_offset(0), value_mul((double)one_scale / 127.0) {
               }
            };

            //(status byte << 8) || num -> mapping
            typedef QHash<uint32_t, mapping_t> mapping_hash_t;
            mapping_hash_t mMappings;
      };
   }
}

#endif
