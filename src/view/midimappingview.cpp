#include "midimapper.hpp"
#include "midimappingview.hpp"
#include "audiomodel.hpp"
#include "defines.hpp"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QString>
#include <QObject>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QPushButton>
#include <iostream>

using namespace dj;
using namespace dj::view;

using std::cerr;
using std::endl;

namespace {
   enum PLAYER_ROWS {
      PLAYER_SIGNAL = 0,
      PLAYER_INDEX = 1,
      PLAYER_TYPE = 2,
      PLAYER_PARAM_NUM = 3,
      PLAYER_CHANNEL = 4,
      PLAYER_MUL = 5,
      PLAYER_OFFSET = 6,
      PLAYER_RESET = 7,
      PLAYER_COLUMN_COUNT = 8
   };

   enum MASTER_ROWS {
      MASTER_SIGNAL = 0,
      MASTER_TYPE = 1,
      MASTER_PARAM_NUM = 2,
      MASTER_CHANNEL = 3,
      MASTER_MUL = 4,
      MASTER_OFFSET = 5,
      MASTER_RESET = 6,
      MASTER_COLUMN_COUNT = 7
   };

}

MIDIMapper::MIDIMapper(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f) {
   mPlayerTable = new QTableWidget;
   mPlayerTable->setColumnCount(PLAYER_COLUMN_COUNT);
   QStringList header_labels;
   header_labels << "signal" << "player" << "type" << "param number" <<  "channel" << "value multiplier" << "value offset" << "reset";
   mPlayerTable->setHorizontalHeaderLabels(header_labels);
   mPlayerTable->verticalHeader()->setVisible(false);

   QStringList signal_types;
   signal_types << "trigger" << "bool" << "int" << "double";

   QStringList signal_and_type;

   QString signal, signal_type;
   foreach(signal_type, signal_types) {
      foreach(signal, audio::AudioModel::player_signals[signal_type]) {
         signal_and_type << signal + "|" + signal_type;
         if (signal_type == "int" || signal_type == "double")
            signal_and_type << signal + "_relative|trigger";
      }
   }

   //sort the list
   signal_and_type.sort();

   QString item;
   foreach(item, signal_and_type) {
      QStringList items = item.split("|");
      signal = items[0];
      signal_type = items[1];
      insert_player_rows(mPlayerTable, signal_type, signal);
   }

   QHBoxLayout * top_layout = new QHBoxLayout;
   top_layout->addWidget(mPlayerTable);
   setLayout(top_layout);
}

MIDIMapper::~MIDIMapper() {
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double value_multiplier,
      double value_offset) {

   QList<QTableWidgetItem *> signal_items = mPlayerTable->findItems(signal_name, Qt::MatchFixedString);
   if (signal_items.isEmpty()) {
      cerr << DJ_FILEANDLINE << signal_name.toStdString() << " not found in table" << endl;
      return; //TODO display error?
   }

   QTableWidgetItem * item;
   foreach(item, signal_items) {
      int row = item->row();
      int index = item->data(Qt::UserRole).toInt();
      if (index == player_index) {
         //type
         QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
         int type_index = combo_box->findData(midi_type);
         if (type_index < 0) {
            cerr << DJ_FILEANDLINE << "invalid midi type given" << endl;
            return;
         }
         combo_box->setCurrentIndex(type_index);

         //channel and param num
         static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL))->setValue(midi_channel + 1);
         static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM))->setValue(midi_param);

         //offset and mult
         static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->setText(QString::number(value_offset));
         static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->setText(QString::number(value_multiplier));

         return;
      }
   }
   cerr << DJ_FILEANDLINE << signal_name.toStdString() << " " << player_index << " not found in table" << endl;
}

void MIDIMapper::default_button_pressed() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   int row = button->property("row").toInt();
   QString signal = mPlayerTable->item(row, PLAYER_SIGNAL)->text();

   double default_offset, default_mult;
   controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);

   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->setText(QString::number(default_offset));
   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->setText(QString::number(default_mult));
}

void MIDIMapper::lineedit_changed(QString /* text */) {
   QLineEdit * item = static_cast<QLineEdit *>(QObject::sender());
   player_row_changed(item->property("row").toInt());
}

void MIDIMapper::combobox_changed(int /* index */) {
   QComboBox * item = static_cast<QComboBox *>(QObject::sender());
   player_row_changed(item->property("row").toInt());
}

void MIDIMapper::spinbox_changed(int /* value */) {
   QSpinBox * item = static_cast<QSpinBox *>(QObject::sender());
   player_row_changed(item->property("row").toInt());
}

void MIDIMapper::insert_player_rows(QTableWidget * table, QString type, QString signal) {
   QTableWidgetItem *item;
   int num_players = static_cast<int>(audio::AudioModel::instance()->player_count());

   for (int i = 0; i < num_players; i++) {
      int row = table->rowCount();
      table->insertRow(row);

      //signal name
      item = new QTableWidgetItem(signal);
      item->setFlags(Qt::ItemIsSelectable);
      item->setData(Qt::UserRole, i);
      table->setItem(row, PLAYER_SIGNAL, item);

      //player index
      item = new QTableWidgetItem(QString::number(i));
      item->setFlags(Qt::ItemIsSelectable);
      item->setData(Qt::UserRole, i);
      table->setItem(row, PLAYER_INDEX, item);

      //midi type
      QComboBox * midi_types = new QComboBox;
      midi_types->addItem("disabled", dj::controller::NO_MAPPING);
      if (type == "trigger") {
         midi_types->addItem("cc", dj::controller::CONTROL_CHANGE);
         midi_types->addItem("note on", dj::controller::NOTE_ON);
      } else if (type == "bool") {
         midi_types->addItem("cc", dj::controller::CONTROL_CHANGE);
         midi_types->addItem("note on", dj::controller::NOTE_ON);
         midi_types->addItem("note toggle", dj::controller::NOTE_TOGGLE);
      } else if (type == "int" || type == "double") {
         midi_types->addItem("cc", dj::controller::CONTROL_CHANGE);
      }
      midi_types->setProperty("row", row);
      QObject::connect(midi_types, SIGNAL(currentIndexChanged(int)), SLOT(combobox_changed(int)));
      table->setCellWidget(row, PLAYER_TYPE, midi_types);

      //param num
      QSpinBox * param_box = new QSpinBox;
      param_box->setRange(0,127);
      param_box->setProperty("row", row);
      QObject::connect(param_box, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
      table->setCellWidget(row, PLAYER_PARAM_NUM, param_box);

      //channel
      QSpinBox * chan_box = new QSpinBox;
      chan_box->setRange(1,16);
      chan_box->setProperty("row", row);
      QObject::connect(chan_box, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
      table->setCellWidget(row, PLAYER_CHANNEL, chan_box);

      QDoubleValidator * double_validator = new QDoubleValidator;

      double default_offset, default_mult;
      controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);

      //multiplier
      QLineEdit * number_edit = new QLineEdit;
      number_edit->setValidator(double_validator);
      number_edit->setText(QString::number(default_mult));
      number_edit->setProperty("row", row);
      QObject::connect(number_edit, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
      table->setCellWidget(row, PLAYER_MUL, number_edit);

      //offset
      number_edit = new QLineEdit;
      number_edit->setValidator(double_validator);
      number_edit->setText(QString::number(default_offset));
      number_edit->setProperty("row", row);
      QObject::connect(number_edit, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
      table->setCellWidget(row, PLAYER_OFFSET, number_edit);

      //default
      QPushButton * default_button = new QPushButton("defaults");
      default_button->setCheckable(false);
      default_button->setProperty("row", row);
      table->setCellWidget(row, PLAYER_RESET, default_button);
      QObject::connect(default_button, SIGNAL(pressed()),
            SLOT(default_button_pressed()));
   }
}

void MIDIMapper::player_row_changed(int row) {
   QString signal_name = mPlayerTable->item(row, PLAYER_SIGNAL)->text();
   int player_index = mPlayerTable->item(row, PLAYER_INDEX)->data(Qt::UserRole).toInt();

   QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
   midimapping_t midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());

   QSpinBox * spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL));
   int midi_channel = spin_box->value() - 1;

   spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM));
   int midi_param = spin_box->value();

   double offset = static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->text().toDouble();
   double mult = static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->text().toDouble();
   emit(player_mapping_update(
            player_index,
            signal_name,
            midi_type, midi_channel, midi_param,
            mult, offset));
}

