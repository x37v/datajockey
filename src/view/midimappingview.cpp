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
#include <QSettings>
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
      MASTER_DELETE,
      MASTER_COLUMN_COUNT
   };

   void set_defaults(QTableWidget * table, int mult_row, int offset_row, const QString& signal, int row);
}

MIDIMapper::MIDIMapper(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f) {
   mDoubleValidator = new QDoubleValidator;

   QStringList signal_types;
   signal_types << "trigger" << "bool" << "int" << "double";

   //build up the signals and types map
   QString signal_type;
   foreach(signal_type, signal_types) {
      QString signal;
      foreach(signal, audio::AudioModel::player_signals[signal_type]) {
         mPlayerSignals[signal] = signal_type;
      }
   }
   foreach(signal_type, signal_types) {
      QString signal;
      foreach(signal, audio::AudioModel::master_signals[signal_type]) {
         mMasterSignals[signal] = signal_type;
      }
   }

   mPlayerTable = new QTableWidget;
   mMasterTable = new QTableWidget;
   mPlayerTable->setColumnCount(PLAYER_COLUMN_COUNT);
   mMasterTable->setColumnCount(MASTER_COLUMN_COUNT);

   QStringList header_labels;
   header_labels << "signal" << "player" << "type" << "param number" <<  "channel" << "value multiplier" << "value offset" << "reset" << "valid" << "delete";
   mPlayerTable->setHorizontalHeaderLabels(header_labels);
   mPlayerTable->verticalHeader()->setVisible(false);

   header_labels.clear();
   header_labels << "signal" << "type" << "param number" <<  "channel" << "value multiplier" << "value offset" << "reset" << "valid" << "delete";
   mMasterTable->setHorizontalHeaderLabels(header_labels);
   mMasterTable->verticalHeader()->setVisible(false);

   QVBoxLayout * player_layout = new QVBoxLayout;
   QVBoxLayout * master_layout = new QVBoxLayout;
   player_layout->addWidget(mPlayerTable);
   master_layout->addWidget(mMasterTable);

   QPushButton * addplayer_button = new QPushButton("add mapping", this);
   addplayer_button->setCheckable(false);
   QObject::connect(addplayer_button, SIGNAL(pressed()), SLOT(add_player_row()));
   player_layout->addWidget(addplayer_button);

   QPushButton * addmaster_button = new QPushButton("add mapping", this);
   addmaster_button->setCheckable(false);
   QObject::connect(addmaster_button, SIGNAL(pressed()), SLOT(add_master_row()));
   master_layout->addWidget(addmaster_button);

   QHBoxLayout * table_layout = new QHBoxLayout;
   table_layout->addLayout(player_layout);
   table_layout->addLayout(master_layout);

   QVBoxLayout * top_layout = new QVBoxLayout;
   top_layout->addLayout(table_layout);

   QHBoxLayout * button_layout = new QHBoxLayout;

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

   read_settings();
}

MIDIMapper::~MIDIMapper() {
}

bool MIDIMapper::validate() {
   QHash<uint32_t, QPair<table_t, int> > key_to_row;
   bool dups = false;

   for (int row = 0; row < mPlayerTable->rowCount(); row++) {
      midimapping_t midi_type;
      int channel, param;
      row_midi_data(PLAYER, row, midi_type, channel, param);

      uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)channel, (uint8_t)param);
      if (key_to_row.contains(key))
         dups = true;
      else //might go invalid later
         mPlayerTable->item(row, PLAYER_VALID)->setText(QString());

      key_to_row.insertMulti(key, QPair<table_t, int>(PLAYER, row));
   }

   for (int row = 0; row < mMasterTable->rowCount(); row++) {
      midimapping_t midi_type;
      int channel, param;
      row_midi_data(MASTER, row, midi_type, channel, param);

      uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)channel, (uint8_t)param);
      if (key_to_row.contains(key))
         dups = true;
      else //might go invalid later
         mMasterTable->item(row, MASTER_VALID)->setText(QString());

      key_to_row.insertMulti(key, QPair<table_t, int>(MASTER, row));
   }

   if (!dups)
      return true;

   //invalidate all the invalid rows
   uint32_t key;
   foreach (key, key_to_row.keys()) {
      QList<QPair<table_t, int> > rows = key_to_row.values(key);
      if (rows.length() > 1) {
         QPair<table_t, int> row;
         foreach(row, rows) {
            if (row.first == PLAYER)
               mPlayerTable->item(row.second, PLAYER_VALID)->setText("invalid");
            else
               mMasterTable->item(row.second, MASTER_VALID)->setText("invalid");
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

void MIDIMapper::closeEvent(QCloseEvent * event) {
   write_settings();
}

void MIDIMapper::write_settings() {
   QSettings settings;
   settings.beginGroup("MIDIMapperView");
   settings.setValue("size", size());
   settings.setValue("pos", pos());
   settings.endGroup();
}

void MIDIMapper::read_settings() {
   QSettings settings;
   settings.beginGroup("MIDIMapperView");
   resize(settings.value("size", QSize(400, 400)).toSize());
   move(settings.value("pos", QPoint(200, 200)).toPoint());
   settings.endGroup();
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
   for (int row = 0; row < mMasterTable->rowCount(); row++)
      send_master_row(row);
   return true;
}

void MIDIMapper::map_player(
      int player_index,
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double value_multiplier, double value_offset) {
   uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)midi_channel, (uint8_t)midi_param);
   int row = find_row_by_key(PLAYER, key);
   if (row < 0)
      row = add_player_row();
   update_row(row,
         player_index,
         signal_name,
         midi_type, midi_channel, midi_param,
         value_multiplier, value_offset);
}

void MIDIMapper::map_master(
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double value_multiplier,
      double value_offset) {
   uint32_t key = controller::MIDIMapper::make_key(midi_type, (uint8_t)midi_channel, (uint8_t)midi_param);
   int row = find_row_by_key(MASTER, key);
   if (row < 0)
      row = add_master_row();
   update_row(row,
         -1,
         signal_name,
         midi_type, midi_channel, midi_param,
         value_multiplier, value_offset);
}

void MIDIMapper::default_button_pressed() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   const QString table_name = button->property("table").toString();
   QTableWidget * table;

   int reset_column, signal_column, offset_column, mul_column;

   if (table_name == "player") {
      table = mPlayerTable;
      reset_column = PLAYER_RESET;
      signal_column = PLAYER_SIGNAL;
      offset_column = PLAYER_OFFSET;
      mul_column = PLAYER_MUL;
   } else {
      table = mMasterTable;
      reset_column = MASTER_RESET;
      signal_column = MASTER_SIGNAL;
      offset_column = MASTER_OFFSET;
      mul_column = MASTER_MUL;
   }

   //find the row
   int row = find_item_table_row(button, reset_column, table);
   if (row < 0)
      return;

   QComboBox * signal_box = static_cast<QComboBox *>(table->cellWidget(row, signal_column));
   QString signal = signal_box->itemText(signal_box->currentIndex());
   set_defaults(table, mul_column, offset_column, signal, row);
}

void MIDIMapper::mapping_signal_changed(int index) {
   QComboBox * signal_box = static_cast<QComboBox *>(QObject::sender());
   const QString signal = signal_box->itemText(index);
   const QString table_name = signal_box->property("table").toString();
   QTableWidget * table;
   QMap<QString, QString> signal_map; //name, type

   int signal_column, type_column, mul_column, offset_column;
   if (table_name == "player") {
      table = mPlayerTable;
      signal_map = mPlayerSignals;
      signal_column = PLAYER_SIGNAL;
      type_column = PLAYER_TYPE;
      offset_column = PLAYER_OFFSET;
      mul_column = PLAYER_MUL;
   } else {
      table = mMasterTable;
      signal_map = mMasterSignals;
      signal_column = MASTER_SIGNAL;
      type_column = MASTER_TYPE;;
      offset_column = MASTER_OFFSET;
      mul_column = MASTER_MUL;
   }

   const QString signal_type = signal_map[signal];

   int row = find_item_table_row(signal_box, signal_column, table);
   if (row < 0)
      return;

   QComboBox * type_box = static_cast<QComboBox *>(table->cellWidget(row, type_column));

   //update the type box
   fillin_type_box(signal, signal_type, type_box);

   //set the defaults
   set_defaults(table, mul_column, offset_column, signal, row);

   validate();
}

void MIDIMapper::lineedit_changed(QString /* text */) {
   validate();
   //QLineEdit * item = static_cast<QLineEdit *>(QObject::sender());
}

void MIDIMapper::combobox_changed(int /* index */) {
   validate();
   //QComboBox * item = static_cast<QComboBox *>(QObject::sender());
}

void MIDIMapper::spinbox_changed(int /* value */) {
   validate();
   //QSpinBox * item = static_cast<QSpinBox *>(QObject::sender());
}

void MIDIMapper::delete_player_row() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   //find the row
   int row = find_item_table_row(button, PLAYER_DELETE, mPlayerTable);
   if (row >= 0)
      delete_row(PLAYER, row);
}

void MIDIMapper::delete_master_row() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   //find the row
   int row = find_item_table_row(button, MASTER_DELETE, mMasterTable);
   if (row >= 0)
      delete_row(MASTER, row);
}

void MIDIMapper::delete_row(table_t type, int row) {
   int player_index, midi_channel, midi_param;
   QString signal_name;
   midimapping_t midi_type ;
   double mult, offset;

   if (type == PLAYER) {
      //get the data
      player_row_mapping_data(row,
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
      mPlayerTable->removeRow(row);
   } else {
      //get the data
      master_row_mapping_data(row,
            signal_name,
            midi_type, midi_channel, midi_param,
            mult, offset);

      //send a removal
      emit(master_mapping_update(
               signal_name,
               dj::controller::NO_MAPPING, midi_channel, midi_param,
               mult, offset));
      mMasterTable->removeRow(row);
   }
   validate();
}

int MIDIMapper::add_player_row() {
   return add_row(PLAYER);
}

int MIDIMapper::add_master_row() {
   return add_row(MASTER);
}

int MIDIMapper::add_row(table_t type) {
   QTableWidget * table;
   QMap<QString, QString> signal_map; //name, type

   QComboBox * signal_name = NULL;
   QComboBox * midi_type = NULL;
   QSpinBox * player_index = NULL;
   QSpinBox * param_num = NULL;
   QSpinBox * channel = NULL;
   QLineEdit * multiplier = NULL;
   QLineEdit * offset = NULL;
   QPushButton * default_button = NULL;
   QPushButton * delete_button = NULL;
   QTableWidgetItem * valid_item = NULL;

   int signal_column;
   int index_column = 0;
   int type_column;
   int param_num_column;
   int channel_column;
   int mul_column;
   int offset_column;
   int reset_column;
   int valid_column;
   int delete_column;
   
   if (type == PLAYER) {
      table = mPlayerTable;
      signal_map = mPlayerSignals;

      signal_column = PLAYER_SIGNAL;
      index_column = PLAYER_INDEX;
      type_column = PLAYER_TYPE;
      param_num_column = PLAYER_PARAM_NUM;
      channel_column = PLAYER_CHANNEL;
      mul_column = PLAYER_MUL;
      offset_column = PLAYER_OFFSET;
      reset_column = PLAYER_RESET;
      valid_column = PLAYER_VALID;
      delete_column = PLAYER_DELETE;
   } else {
      table = mMasterTable;
      signal_map = mMasterSignals;

      signal_column = MASTER_SIGNAL;
      type_column = MASTER_TYPE;
      param_num_column = MASTER_PARAM_NUM;
      channel_column = MASTER_CHANNEL;
      mul_column = MASTER_MUL;
      offset_column = MASTER_OFFSET;
      reset_column = MASTER_RESET;
      valid_column = MASTER_VALID;
      delete_column = MASTER_DELETE;
   }

   int row = table->rowCount();
   table->insertRow(row);

   //signal name
   signal_name = new QComboBox;
   signal_name->setProperty("table", type == PLAYER ? "player" : "master");
   QString signal;
   foreach(signal, signal_map.keys()) {
      signal_name->addItem(signal);
   }

   //set the signal to the first signal in the list
   signal = signal_map.keys().first();
   table->setCellWidget(row, signal_column, signal_name);

   //player index
   if (type == PLAYER) {
      player_index = new QSpinBox;
      player_index->setRange(1, audio::AudioModel::instance()->player_count());
      table->setCellWidget(row, index_column, player_index);
   }

   //midi type
   midi_type = new QComboBox;
   fillin_type_box(signal, signal_map.values().first(), midi_type);
   table->setCellWidget(row, type_column, midi_type);

   //param num
   param_num = new QSpinBox;
   param_num->setRange(0,127);
   table->setCellWidget(row, param_num_column, param_num);

   //channel
   channel = new QSpinBox;
   channel->setRange(1,16);
   table->setCellWidget(row, channel_column, channel);

   double default_offset, default_mult;
   controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);

   //multiplier
   multiplier = new QLineEdit;
   multiplier->setValidator(mDoubleValidator);
   multiplier->setText(QString::number(default_mult));
   table->setCellWidget(row, mul_column, multiplier);

   //offset
   offset = new QLineEdit;
   offset->setValidator(mDoubleValidator);
   offset->setText(QString::number(default_offset));
   table->setCellWidget(row, offset_column, offset);

   //default
   default_button = new QPushButton("defaults");
   default_button->setCheckable(false);
   default_button->setProperty("table", type == PLAYER ? "player" : "master");
   table->setCellWidget(row, reset_column, default_button);

   //valid
   valid_item = new QTableWidgetItem(QString());
   valid_item->setFlags(Qt::NoItemFlags);
   valid_item->setData(Qt::UserRole, row);
   table->setItem(row, valid_column, valid_item);

   //delete
   delete_button = new QPushButton("delete");
   delete_button->setCheckable(false);
   delete_button->setProperty("table", type == PLAYER ? "player" : "master");
   table->setCellWidget(row, delete_column, delete_button);

   set_defaults(table, mul_column, offset_column, signal, row);

   if (type == PLAYER) {
      QObject::connect(delete_button, SIGNAL(pressed()), SLOT(delete_player_row()));
      QObject::connect(player_index, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   } else {
      QObject::connect(delete_button, SIGNAL(pressed()), SLOT(delete_master_row()));
   }

   //generic
   QObject::connect(default_button, SIGNAL(pressed()), SLOT(default_button_pressed()));
   QObject::connect(signal_name, SIGNAL(currentIndexChanged(int)), SLOT(mapping_signal_changed(int)));
   QObject::connect(multiplier, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
   QObject::connect(offset, SIGNAL(textChanged(QString)), SLOT(lineedit_changed(QString)));
   QObject::connect(channel, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   QObject::connect(param_num, SIGNAL(valueChanged(int)), SLOT(spinbox_changed(int)));
   QObject::connect(midi_type, SIGNAL(currentIndexChanged(int)), SLOT(combobox_changed(int)));

   return row;
}

void MIDIMapper::send_player_row(int row) {
   int player_index, midi_channel, midi_param;
   QString signal_name;
   midimapping_t midi_type ;
   double mult, offset;

   //get the data
   player_row_mapping_data(row,
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

void MIDIMapper::send_master_row(int row) {
   int midi_channel, midi_param;
   QString signal_name;
   midimapping_t midi_type ;
   double mult, offset;

   //get the data
   master_row_mapping_data(row,
         signal_name,
         midi_type, midi_channel, midi_param,
         mult, offset);

   //send it
   emit(master_mapping_update(
            signal_name,
            midi_type, midi_channel, midi_param,
            mult, offset));
}

void MIDIMapper::fillin_type_box(const QString& signal_name, const QString& signal_type, QComboBox * type_box) {
   if (type_box->property("signal_type").toString() != signal_type) {
      //remove all
      while(type_box->count() > 0)
         type_box->removeItem(0);

      //build up a new one
      if (signal_type == "trigger" || signal_name.contains("relative")) {
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

int MIDIMapper::find_row_by_key(table_t type, uint32_t key) {
   QTableWidget * table = (type == PLAYER ? mPlayerTable : mMasterTable);
   for (int row = 0; row < table->rowCount(); row++) {
      midimapping_t midi_type;
      int channel, param;
      row_midi_data(type, row, midi_type, channel, param);
      uint32_t row_key = controller::MIDIMapper::make_key(midi_type, (uint8_t)channel, (uint8_t)param);
      if (key == row_key)
         return row;
   }
   return -1;
}

void MIDIMapper::row_midi_data(table_t type, int row, midimapping_t &midi_type, int &midi_channel, int &midi_param) {
   if (type == PLAYER) {
      QComboBox * combo_box = static_cast<QComboBox*>(mPlayerTable->cellWidget(row, PLAYER_TYPE));
      midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());
      midi_channel = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_CHANNEL))->value() - 1;
      midi_param = static_cast<QSpinBox*>(mPlayerTable->cellWidget(row, PLAYER_PARAM_NUM))->value();
   } else {
      QComboBox * combo_box = static_cast<QComboBox*>(mMasterTable->cellWidget(row, MASTER_TYPE));
      midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());
      midi_channel = static_cast<QSpinBox*>(mMasterTable->cellWidget(row, MASTER_CHANNEL))->value() - 1;
      midi_param = static_cast<QSpinBox*>(mMasterTable->cellWidget(row, MASTER_PARAM_NUM))->value();
   }
}

void MIDIMapper::player_row_mapping_data(int row,
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

void MIDIMapper::master_row_mapping_data(int row,
      QString& signal_name,
      midimapping_t& midi_type, int& midi_channel, int& midi_param,
      double& mult, double& offset) {
   //signal
   QComboBox * combo_box = static_cast<QComboBox*>(mMasterTable->cellWidget(row, MASTER_SIGNAL));
   signal_name = combo_box->itemText(combo_box->currentIndex());

   //type
   combo_box = static_cast<QComboBox*>(mMasterTable->cellWidget(row, MASTER_TYPE));
   midi_type = static_cast<midimapping_t>(combo_box->itemData(combo_box->currentIndex()).toInt());

   //channel
   QSpinBox *spin_box = static_cast<QSpinBox*>(mMasterTable->cellWidget(row, MASTER_CHANNEL));
   midi_channel = spin_box->value() - 1;

   //param
   spin_box = static_cast<QSpinBox*>(mMasterTable->cellWidget(row, MASTER_PARAM_NUM));
   midi_param = spin_box->value();

   //offset/mul
   offset = static_cast<QLineEdit*>(mMasterTable->cellWidget(row, MASTER_OFFSET))->text().toDouble();
   mult = static_cast<QLineEdit*>(mMasterTable->cellWidget(row, MASTER_MUL))->text().toDouble();
}

void MIDIMapper::update_row(int row,
      int player_index,
      QString signal_name,
      midimapping_t midi_type, int midi_channel, int midi_param,
      double mult, double offset) {

   QTableWidget * table;
   QMap<QString, QString> signal_map; //name, type

   int signal_column;
   int index_column = 0;
   int type_column;
   int param_num_column;
   int channel_column;
   int mul_column;
   int offset_column;
   
   if (player_index >= 0) {
      table = mPlayerTable;
      signal_map = mPlayerSignals;

      signal_column = PLAYER_SIGNAL;
      index_column = PLAYER_INDEX;
      type_column = PLAYER_TYPE;
      param_num_column = PLAYER_PARAM_NUM;
      channel_column = PLAYER_CHANNEL;
      mul_column = PLAYER_MUL;
      offset_column = PLAYER_OFFSET;
   } else {
      table = mMasterTable;
      signal_map = mMasterSignals;

      signal_column = MASTER_SIGNAL;
      type_column = MASTER_TYPE;
      param_num_column = MASTER_PARAM_NUM;
      channel_column = MASTER_CHANNEL;
      mul_column = MASTER_MUL;
      offset_column = MASTER_OFFSET;
   }

   //signal
   QComboBox * combo_box = static_cast<QComboBox*>(table->cellWidget(row, signal_column));
   int index = combo_box->findText(signal_name);
   if (index < 0) {
      cerr << DJ_FILEANDLINE << signal_name.toStdString() << " not found in signal list" << endl;
      return;
   }
   combo_box->setCurrentIndex(index);

   QSpinBox * spin_box = NULL;
   if (player_index >= 0) {
      //index
      spin_box = static_cast<QSpinBox*>(table->cellWidget(row, index_column));
      spin_box->setValue(player_index + 1);
   }

   //update the type box
   const QString signal_type = signal_map[signal_name];
   QComboBox * type_box = static_cast<QComboBox *>(table->cellWidget(row, type_column));
   fillin_type_box(signal_name, signal_type, type_box);
   index = type_box->findData(midi_type);
   if (index < 0) {
      cerr << DJ_FILEANDLINE << " problem finding type in type box " << signal_type.toStdString() << endl;
      return;
   }
   type_box->setCurrentIndex(index);

   //channel
   spin_box = static_cast<QSpinBox*>(table->cellWidget(row, channel_column));
   spin_box->setValue(midi_channel + 1);

   //param
   spin_box = static_cast<QSpinBox*>(table->cellWidget(row, param_num_column));
   spin_box->setValue(midi_param);

   //offset/mul
   static_cast<QLineEdit*>(table->cellWidget(row, offset_column))->setText(QString::number(offset));
   static_cast<QLineEdit*>(table->cellWidget(row, mul_column))->setText(QString::number(mult));
}

namespace {
   void set_defaults(QTableWidget * table, int mult_row, int offset_row, const QString& signal, int row) {
      double default_offset, default_mult;
      controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);
      static_cast<QLineEdit*>(table->cellWidget(row, mult_row))->setText(QString::number(default_mult));
      static_cast<QLineEdit*>(table->cellWidget(row, offset_row))->setText(QString::number(default_offset));
   }
}

