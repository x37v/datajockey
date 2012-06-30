#ifndef DJ_MIDIMAPPING_VIEW_HPP
#define DJ_MIDIMAPPING_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QTableWidget>
#include "midimapper.hpp"

class QComboBox;
class QDoubleValidator;

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
            enum table_t {
               PLAYER,
               MASTER
            };
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
            void delete_player_row();
            void delete_master_row();
            int add_player_row();
            int add_master_row();
         private:
            void delete_row(table_t type, int row);
            int add_row(table_t type);

            QTableWidget * mPlayerTable;
            QTableWidget * mMasterTable;
            QMap<QString, QString> mPlayerSignals; //name, type
            QMap<QString, QString> mMasterSignals; //name, type
            QDoubleValidator * mDoubleValidator;

            void send_player_row(int row);
            void send_master_row(int row);

            void fillin_type_box(const QString& signal_name, const QString& signal_type, QComboBox * type_box);
            int find_row_by_key(table_t type, uint32_t key);

            void row_midi_data(table_t type, int row, midimapping_t &midi_type, int &midi_channel, int &midi_param);

            void player_row_mapping_data(int row,
                     int& player_index,
                     QString& signal_name,
                     midimapping_t& midi_type, int& midi_channel, int& midi_param,
                     double& mult, double& offset);
            void master_row_mapping_data(int row,
                     QString& signal_name,
                     midimapping_t& midi_type, int& midi_channel, int& midi_param,
                     double& mult, double& offset);

            //if you send a negative value as player_index this will address the master table
            void update_row(int row,
                     int player_index,
                     QString signal_name,
                     midimapping_t midi_type, int midi_channel, int midi_param,
                     double mult, double offset);

            template <typename T>
               static int find_item_table_row(T item, int item_column, QTableWidget * table) {
                  for (int i = 0; i < table->rowCount(); i++) {
                     if (static_cast<T>(table->cellWidget(i, item_column)) == item)
                        return i;
                  }
                  return -1;
               }

      };
   }
}

#endif

