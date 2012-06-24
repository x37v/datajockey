#include "midimappingview.hpp"
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHBoxLayout>
#include <QString>
#include <QObject>
#include <cmath>

using namespace dj::view;

namespace {
   void insert_row(QTableWidget * table) {
      int row = table->rowCount();
      table->insertRow(row);

      for (int column = 0; column < 8; column++) {
         QTableWidgetItem *newItem = new QTableWidgetItem(QString("%1").arg(pow(row, column + 1)));
         table->setItem(row, column, newItem);
      }
   }
}

MIDIMapper::MIDIMapper(QWidget * parent, Qt::WindowFlags f) : QWidget(parent, f) {
   mPlayerTable = new QTableWidget;
   mPlayerTable->setColumnCount(8);
   for(int i = 0; i < 10; i++)
      insert_row(mPlayerTable);

   QHBoxLayout * top_layout = new QHBoxLayout;
   top_layout->addWidget(mPlayerTable);
   setLayout(top_layout);
}

MIDIMapper::~MIDIMapper() {
}

