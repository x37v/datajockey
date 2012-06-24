#include "midimapper.hpp"
#include "midimappingview.hpp"
#include "audiomodel.hpp"
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

using namespace dj;
using namespace dj::view;

namespace {
   void insert_player_rows(QTableWidget * table, QString type, QString signal) {
      QTableWidgetItem *item;
      int num_players = static_cast<int>(audio::AudioModel::instance()->player_count());

      for (int i = 0; i < num_players; i++) {
         int row = table->rowCount();
         table->insertRow(row);

         //signal name
         item = new QTableWidgetItem(signal);
         item->setFlags(Qt::ItemIsSelectable);
         table->setItem(row, 0, item);

         //player index
         item = new QTableWidgetItem(QString::number(i));
         item->setFlags(Qt::ItemIsSelectable);
         table->setItem(row, 1, item);

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
         table->setCellWidget(row, 2, midi_types);

         //param num
         QSpinBox * param_box = new QSpinBox;
         param_box->setRange(0,127);
         table->setCellWidget(row, 3, param_box);

         //channel
         QSpinBox * chan_box = new QSpinBox;
         chan_box->setRange(1,16);
         table->setCellWidget(row, 4, chan_box);

         QDoubleValidator * double_validator = new QDoubleValidator;

         double default_offset, default_mult;
         controller::MIDIMapper::default_value_mappings(signal, default_offset, default_mult);

         //multiplier
         QLineEdit * number_edit = new QLineEdit;
         number_edit->setValidator(double_validator);
         number_edit->setText(QString::number(default_mult));
         table->setCellWidget(row, 5, number_edit);

         //offset
         number_edit = new QLineEdit;
         number_edit->setValidator(double_validator);
         number_edit->setText(QString::number(default_offset));
         table->setCellWidget(row, 6, number_edit);

         //default
         QPushButton * default_button = new QPushButton("default");
         default_button->setCheckable(false);
         table->setCellWidget(row, 7, default_button);
      }
   }
}

MIDIMapper::MIDIMapper(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f) {
   mPlayerTable = new QTableWidget;
   mPlayerTable->setColumnCount(8);
   QStringList header_labels;
   header_labels << "signal" << "player" << "type" << "param number" <<  "channel" << "value multiplier" << "value offset" << "reset";
   mPlayerTable->setHorizontalHeaderLabels(header_labels);
   mPlayerTable->verticalHeader()->setVisible(false);

   QStringList signal_types;
   signal_types << "trigger" << "bool" << "int" << "double";

   QString signal, signal_type;
   foreach(signal_type, signal_types) {
      foreach(signal, audio::AudioModel::player_signals[signal_type]) {
         insert_player_rows(mPlayerTable, signal_type, signal);
         if (signal_type == "int" || signal_type == "double")
            insert_player_rows(mPlayerTable, "trigger", signal + "_relative");
      }
   }
   mPlayerTable->sortByColumn(0, Qt::AscendingOrder);

   QHBoxLayout * top_layout = new QHBoxLayout;
   top_layout->addWidget(mPlayerTable);
   setLayout(top_layout);
}

MIDIMapper::~MIDIMapper() {
}

