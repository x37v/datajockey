#include "midimapper.hpp"
#include "midimappingview.hpp"
#include "audiomodel.hpp"
#include "defines.hpp"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>
#include <QObject>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <iostream>

using namespace dj;
using namespace dj::view;

using std::cerr;
using std::endl;

namespace {
   enum PLAYER_ROWS {
      PLAYER_SIGNAL = 0,
      PLAYER_INDEX,
      PLAYER_TYPE,
      PLAYER_PARAM_NUM,
      PLAYER_CHANNEL,
      PLAYER_MUL,
      PLAYER_OFFSET,
      PLAYER_RESET,
      PLAYER_VALID,
      PLAYER_DELETE,
      PLAYER_COLUMN_COUNT
   };

   enum MASTER_ROWS {
      MASTER_SIGNAL = 0,
      MASTER_TYPE,
      MASTER_PARAM_NUM,
      MASTER_CHANNEL,
      MASTER_MUL,
      MASTER_OFFSET,
      MASTER_RESET,
      MASTER_VALID,
      MASTER_COLUMN_COUNT
   };

}

MIDIMapper::MIDIMapper(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f) {
   QStringList signal_types;
   signal_types << "trigger" << "bool" << "int" << "double";

   //build up the signals and types map
   QString signal_type;
   foreach(signal_type, signal_types) {
      QString signal;
      foreach(signal, audio::AudioModel::player_signals[signal_type]) {
         mPlayerSignals[signal] = signal_type;
         if (signal_type == "int" || signal_type == "double")
            mPlayerSignals[signal + "_relative"] = "trigger";
      }
   }

   mPlayerTable = new QTableWidget;
   mPlayerTable->setColumnCount(PLAYER_COLUMN_COUNT);

   QStringList header_labels;
   header_labels << "signal" << "player" << "type" << "param number" <<  "channel" << "value multiplier" << "value offset" << "reset" << "valid" << "delete";
   mPlayerTable->setHorizontalHeaderLabels(header_labels);
   mPlayerTable->verticalHeader()->setVisible(false);

   QVBoxLayout * top_layout = new QVBoxLayout;
   top_layout->addWidget(mPlayerTable);

   QHBoxLayout * button_layout = new QHBoxLayout;

   QPushButton * addplayer_button = new QPushButton("add mapping", this);
   addplayer_button->setCheckable(false);
   QObject::connect(addplayer_button, SIGNAL(pressed()), SLOT(add_player_row()));
   button_layout->addWidget(addplayer_button);

   QPushButton * apply_button = new QPushButton("apply", this);
   apply_button->setCheckable(false);
   QObject::connect(apply_button, SIGNAL(pressed()), SLOT(apply()));
   button_layout->addWidget(apply_button);

   QPushButton * okay_button = new QPushButton("okay", this);
   okay_button->setCheckable(false);
   QObject::connect(okay_button, SIGNAL(pressed()), SLOT(okay()));
   button_layout->addWidget(okay_button);

   QPushButton * cancel_button = new QPushButton("cancel", this);
   cancel_button->setCheckable(false);
   QObject::connect(cancel_button, SIGNAL(pressed()), SLOT(close()));
   button_layout->addWidget(cancel_button);


   top_layout->addLayout(button_layout);

   setLayout(top_layout);
}

MIDIMapper::~MIDIMapper() {
}

bool MIDIMapper::validate() {
   QHash<uint32_t, int> key_to_row;
   bool dups = false;

   for (int row = 0; row < mPlayerTable->rowCount(); row++) {
      midimapping_t midi_type;
      int channel, param;
      row_midi_data(row, midi_type, channel, param);

      uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)channel, (uint8_t)param);
      if (key_to_row.contains(key))
         dups = true;
      else //might go invalid later
         mPlayerTable->item(row, PLAYER_VALID)->setText(QString());

      key_to_row.insertMulti(key, row);
   }

   if (!dups)
      return true;

   //invalidate all the invalid rows
   uint32_t key;
   foreach (key, key_to_row.keys()) {
      QList<int> rows = key_to_row.values(key);
      if (rows.length() > 1) {
         int row;
         foreach(row, rows) {
            mPlayerTable->item(row, PLAYER_VALID)->setText("invalid");
         }
      }
   }

   return false;
}

void MIDIMapper::showEvent(QShowEvent * event) {
   QWidget::showEvent(event);
   while(mPlayerTable->rowCount() > 0)
      mPlayerTable->removeRow(0);
   emit(requesting_mappings());
}

void MIDIMapper::okay() {
   if (apply())
      close();
}

bool MIDIMapper::apply() {
   if (!validate()) {
      QMessageBox::information(this,
            "invalid mappings",
            "Your midi mappings contain duplicate/invalid mappings, please resolve those and try again.");
      return false;
   }

   for (int row = 0; row < mPlayerTable->rowCount(); row++)
      send_player_row(row);
   return true;
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double value_multiplier, double value_offset) {
   uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)midi_channel, (uint8_t)midi_param);
   int row = find_player_row(key);
   if (row < 0)
      row = add_player_row();
   update_row(row,
         player_index,
         signal_name,
         midi_type, midi_channel, midi_param,
         value_multiplier, value_offset);
}

void MIDIMapper::default_button_pressed() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   int row = button->property("row").toInt();

   QComboBox * signal_box = static_cast<QComboBox *>(mPlayerTable->cellWidget(row, PLAYER_SIGNAL));
   QString signal = signal_box->itemText(signal_box->currentIndex());
   set_defaults(signal, row);
}

void MIDIMapper::mapping_signal_changed(int index) {
   QComboBox * signal_box = static_cast<QComboBox *>(QObject::sender());
   const QString signal = signal_box->itemText(index);
   const QString signal_type = mPlayerSignals[signal];

   //XXX can we do this the whole time?
   int row = signal_box->property("row").toInt();
   QComboBox * type_box = static_cast<QComboBox *>(mPlayerTable->cellWidget(row, PLAYER_TYPE));

   //update the type box
   fillin_type_box(signal_type, type_box);
   //set the defaults
   set_defaults(signal, row);

   validate();
}

void MIDIMapper::lineedit_changed(QString /* text */) {
   validate();
   //QLineEdit * item = static_cast<QLineEdit *>(QObject::sender());
   //send_player_row(item->property("row").toInt());
}

void MIDIMapper::combobox_changed(int /* index */) {
   validate();
   //QComboBox * item = static_cast<QComboBox *>(QObject::sender());
   //send_player_row(item->property("row").toInt());
}

void MIDIMapper::spinbox_changed(int /* value */) {
   validate();
   //QSpinBox * item = static_cast<QSpinBox *>(QObject::sender());
   //send_player_row(item->property("row").toInt());
}

void MIDIMapper::delete_row() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   int row = button->property("row").toInt();

   int player_index, midi_channel, midi_param;
   QString signal_name;
   midimapping_t midi_type ;
   double mult, offset;

   //get the data
   row_mapping_data(row,
         player_index,
         signal_name,
         midi_type, midi_channel, midi_param,
         mult, offset);

   //send a removal
   emit(player_mapping_update(
            player_index,
            signal_name,
            dj::controller::NO_MAPPING, midi_channel, midi_param,
            mult, offset));

   //we remove all and request..
   //TODO is there a better way?
   while(mPlayerTable->rowCount() > 0)
      mPlayerTable->removeRow(0);
   emit(requesting_mappings());
}

int MIDIMapper::add_player_row() {
   QTableWidgetItem *item;
   QComboBox * combo_box;
   QSpinBox * spin_box;

   int row = mPlayerTable->rowCount();
   mPlayerTable->insertRow(row);

   //signal name
   combo_box = new QComboBox;
   combo_box->setProperty("row", row);
   QString signal;
   foreach(signal, mPlayerSignals.keys()) {
      combo_box->addItem(signal);
   }
   //set the signal to the first signal in the list
   signal = mPlayerSignals.values().first();
   QObject::connect(combo_box, SIGNAL(currentIndexChanged(int)), SLOT(mapping_signal_changed(int)));
   mPlayerTable->setCellWidget(row, PLAYER_SIGNAL, combo_box);

   //player index
   spin_box = new QSpinBox;
   spin_box->setRange(1, audio::AudioModel::instance()->player_count());
   QObject::connect(spin_box, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   mPlayerTable->setCellWidget(row, PLAYER_INDEX, spin_box);

   //midi type
   combo_box = new QComboBox;
   fillin_type_box(signal, combo_box);
   combo_box->setProperty("row", row);
   QObject::connect(combo_box, SIGNAL(currentIndexChanged(int)), SLOT(combobox_changed(int)));
   mPlayerTable->setCellWidget(row, PLAYER_TYPE, combo_box);

   //param num
   spin_box = new QSpinBox;
   spin_box->setRange(0,127);
   QObject::connect(spin_box, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   mPlayerTable->setCellWidget(row, PLAYER_PARAM_NUM, spin_box);

   //channel
   spin_box = new QSpinBox;
   spin_box->setRange(1,16);
   QObject::connect(spin_box, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   mPlayerTable->setCellWidget(row, PLAYER_CHANNEL, spin_box);

   QDoubleValidator * double_validator = new QDoubleValidator;

   double default_offset, default_mult;
   controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);

   //multiplier
   QLineEdit * number_edit = new QLineEdit;
   number_edit->setValidator(double_validator);
   number_edit->setText(QString::number(default_mult));
   QObject::connect(number_edit, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
   mPlayerTable->setCellWidget(row, PLAYER_MUL, number_edit);

   //offset
   number_edit = new QLineEdit;
   number_edit->setValidator(double_validator);
   number_edit->setText(QString::number(default_offset));
   QObject::connect(number_edit, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
   mPlayerTable->setCellWidget(row, PLAYER_OFFSET, number_edit);

   //default
   QPushButton * default_button = new QPushButton("defaults");
   default_button->setProperty("row", row);
   default_button->setCheckable(false);
   mPlayerTable->setCellWidget(row, PLAYER_RESET, default_button);
   QObject::connect(default_button, SIGNAL(pressed()), SLOT(default_button_pressed()));

   //valid
   item = new QTableWidgetItem(QString());
   item->setFlags(Qt::NoItemFlags);
   item->setData(Qt::UserRole, row);
   mPlayerTable->setItem(row, PLAYER_VALID, item);

   //delete
   QPushButton * delete_button = new QPushButton("delete");
   default_button->setProperty("row", row);
   default_button->setCheckable(false);
   mPlayerTable->setCellWidget(row, PLAYER_DELETE, delete_button);
   QObject::connect(delete_button, SIGNAL(pressed()), SLOT(delete_row()));

   set_defaults(signal, row);

   return row;
}

void MIDIMapper::send_player_row(int row) {
   int player_index, midi_channel, midi_param;
   QString signal_name;
   midimapping_t midi_type ;
   double mult, offset;

   //get the data
   row_mapping_data(row,
         player_index,
         signal_name,
         midi_type, midi_channel, midi_param,
         mult, offset);

   //send it
   emit(player_mapping_update(
            player_index,
            signal_name,
            midi_type, midi_channel, midi_param,
            mult, offset));
}

void MIDIMapper::fillin_type_box(const QString& signal_type, QComboBox * type_box) {
   if (type_box->property("signal_type").toString() != signal_type) {
      //remove all
      while(type_box->count() > 0)
         type_box->removeItem(0);

      //build up a new one
      if (signal_type == "trigger") {
         type_box->addItem("cc", dj::controller::CONTROL_CHANGE);
         type_box->addItem("note on", dj::controller::NOTE_ON);
      } else if (signal_type == "bool") {
         type_box->addItem("cc", dj::controller::CONTROL_CHANGE);
         type_box->addItem("note on", dj::controller::NOTE_ON);
         type_box->addItem("note toggle", dj::controller::NOTE_TOGGLE);
      } else if (signal_type == "int" || signal_type == "double") {
         type_box->addItem("cc", dj::controller::CONTROL_CHANGE);
      }
      //store our type
      type_box->property("signal_type").toString() = signal_type;
   }
}

void MIDIMapper::set_defaults(const QString& signal, int row) {
   double default_offset, default_mult;
   controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);
   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->setText(QString::number(default_offset));
   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->setText(QString::number(default_mult));
}

int MIDIMapper::find_player_row(uint32_t key) {
   for (int row = 0; row < mPlayerTable->rowCount(); row++) {
      midimapping_t midi_type;
      int channel, param;
      row_midi_data(row, midi_type, channel, param);
      uint32_t row_key = controller::MIDIMapper::make_key(midi_type, (uint8_t)channel, (uint8_t)param);
      if (key == row_key)
         return row;
   }
   return -1;
}

void MIDIMapper::row_midi_data(int row, midimapping_t &midi_type, int &midi_channel, int &midi_param) {
   QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
   midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());
   midi_channel = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL))->value() - 1;
   midi_param = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM))->value();
}

void MIDIMapper::row_mapping_data(int row,
      int& player_index,
      QString& signal_name,
      midimapping_t& midi_type, int& midi_channel, int& midi_param,
      double& mult, double& offset) {
   //signal
   QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_SIGNAL));
   signal_name = combo_box->itemText(combo_box->currentIndex());

   //index
   QSpinBox * spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_INDEX));
   player_index = spin_box->value() - 1;

   //type
   combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
   midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());

   //channel
   spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL));
   midi_channel = spin_box->value() - 1;

   //param
   spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM));
   midi_param = spin_box->value();

   //offset/mul
   offset = static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->text().toDouble();
   mult = static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->text().toDouble();
}

void MIDIMapper::update_row(int row,
      int player_index,
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double mult, double offset) {
   //signal
   QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_SIGNAL));
   int index = combo_box->findText(signal_name);
   if (index < 0) {
      cerr << DJ_FILEANDLINE << signal_name.toStdString() << " not found in signal list" << endl;
      return;
   }
   combo_box->setCurrentIndex(index);

   //index
   QSpinBox * spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_INDEX));
   spin_box->setValue(player_index + 1);

   //update the type box
   const QString signal_type = mPlayerSignals[signal_name];
   QComboBox * type_box = static_cast<QComboBox *>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
   fillin_type_box(signal_type, type_box);
   index = type_box->findData(midi_type);
   if (index < 0) {
      cerr << DJ_FILEANDLINE << " problem finding type in type box " << signal_type.toStdString() << endl;
      return;
   }
   type_box->setCurrentIndex(index);

   //channel
   spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL));
   spin_box->setValue(midi_channel + 1);

   //param
   spin_box = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM));
   spin_box->setValue(midi_param);

   //offset/mul
   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_OFFSET))->setText(QString::number(offset));
   static_cast<QLineEdit*>(mPlayerTable->cellWidget(row, PLAYER_MUL))->setText(QString::number(mult));
}
