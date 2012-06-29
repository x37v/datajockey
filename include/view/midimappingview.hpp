#ifndef DJ_MIDIMAPPING_VIEW_HPP
#define DJ_MIDIMAPPING_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include "midimapper.hpp"

class QTableWidget;
class QComboBox;

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

            void map_player(
                  int player_index,
                  QString signal_name,
                  midimapping_t midi_type, int midi_channel, int midi_param,
                  double value_multiplier,
                  double value_offset);
            void map_master(
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
            void mapping_signal_changed(int index);
            void lineedit_changed(QString text);
            void combobox_changed(int index);
            void spinbox_changed(int index);
            void delete_row();
            int add_player_row();
            int add_master_row();

         private:
            QTableWidget * mPlayerTable;
            QTableWidget * mMasterTable;
            QMap<QString, QString> mPlayerSignals; //name, type
            QMap<QString, QString> mMasterSignals; //name, type

            void send_player_row(int row);
            void fillin_type_box(const QString& signal_type, QComboBox * type_box);
            void set_defaults(const QString& signal, int row);
            int find_player_row(uint32_t key);
            void row_midi_data(int row, midimapping_t &midi_type, int &midi_channel, int &midi_param);
            void row_mapping_data(int row,
                     int& player_index,
                     QString& signal_name,
                     midimapping_t& midi_type, int& midi_channel, int& midi_param,
                     double& mult, double& offset);
            void update_row(int row,
                     int player_index,
                     QString signal_name,
                     midimapping_t midi_type, int midi_channel, int midi_param,
                     double mult, double offset);
      };
   }
}

#endif

