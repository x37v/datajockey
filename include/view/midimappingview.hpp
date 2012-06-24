#ifndef DJ_MIDIMAPPING_VIEW_HPP
#define DJ_MIDIMAPPING_VIEW_HPP

#include <QWidget>
#include "midimapper.hpp"

class QTableWidget;


namespace dj {
   namespace view {
      using dj::controller::midimapping_t;
      class MIDIMapper : public QWidget {
         Q_OBJECT
         public:
            MIDIMapper(QWidget * parent = NULL, Qt::WindowFlags f = 0);
            ~MIDIMapper();
            bool validate();
         protected:
            virtual void showEvent(QShowEvent * event);
         public slots:
            void okay();
            bool apply();

            void reset();
            void map_player(
                  int player_index,
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);
         signals:
            void requesting_mappings();
            void player_mapping_update(
                  int player_index,
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);
            void master_mapping_update(
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);

         private slots:
            void default_button_pressed();
            void lineedit_changed(QString text);
            void combobox_changed(int index);
            void spinbox_changed(int index);
         private:
            QTableWidget * mPlayerTable;
            void insert_player_rows(QTableWidget * table, QString type, QString signal);
            void send_player_row(int row);
      };
   }
}

#endif

