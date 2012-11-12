/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "workdbview.hpp"
#include "worktablemodel.hpp"
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QAbstractItemModel>
#include <QSettings>
#include <QTimer>

WorkDBView::WorkDBView(QAbstractItemModel * model, 
    QWidget *parent) :
  QWidget(parent)
{
  //create the layouts
  QVBoxLayout * layout = new QVBoxLayout(this);

  //create and set up the tableview
  mTableView = new QTableView(this);
  mTableView->setSortingEnabled(true);
  mTableView->setModel(model);
  mTableView->setColumnHidden(0, true);
  mTableView->horizontalHeader()->setMovable(true);
  //mTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
  mTableView->verticalHeader()->setVisible(false);
  mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTableView->setSelectionMode(QAbstractItemView::SingleSelection);
  //XXX actually do something with editing at some point
  mTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(1);

  layout->addWidget(mTableView, 10);
  setLayout(layout);

  //connect up our internal signals
  QObject::connect(mTableView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection, const QItemSelection)),
      this,
      SLOT(setSelection(const QItemSelection)));

  //schedule the settings reading to happen after view construction
  QTimer::singleShot(0, this, SLOT(read_settings()));
}

QTableView * WorkDBView::tableView(){
  return mTableView;
}

void WorkDBView::selectWork(int work_id){
  //get the first index
  int rows = mTableView->model()->rowCount();
  //iterate to find our work
  for(int i = 0; i < rows; i++){
    QModelIndex index = mTableView->model()->index(i,WorkTableModel::idColumn);
    QVariant data = index.data();
    if(data.isValid() && data.canConvert(QVariant::Int) && data.toInt() == work_id){
      mTableView->selectRow(index.row());
      emit(workSelected(work_id));
      return;
    }
  }
}

/*
   void WorkDBView::selectWork(const QModelIndex & index ){
   QSqlRecord record = ((QSqlTableModel *)mTableView->model())->record(index.row());
   QVariant itemData = record.value(WorkTableModel::idColumn);
//find the id of the work and emit that
if(itemData.isValid() && itemData.canConvert(QVariant::Int)){
int work = itemData.toInt();
emit(workSelected(work));
}
}
*/

void WorkDBView::setSelection( const QItemSelection & selected){
  Q_UNUSED(selected);
  QModelIndex index = mTableView->selectionModel()->currentIndex(); 
  index = index.sibling(index.row(), WorkTableModel::idColumn);
  int work_id = -1;
  if(index.isValid())
    work_id = mTableView->model()->data(index).toInt();
  emit(workSelected(work_id));
}

void WorkDBView::write_settings() {
  QSettings settings;
  settings.beginGroup("WorkDBView");
  settings.setValue("header_state", mTableView->horizontalHeader()->saveState());
  settings.endGroup();
}

void WorkDBView::read_settings() {
  QSettings settings;
  settings.beginGroup("WorkDBView");
  if (settings.contains("header_state"))
    mTableView->horizontalHeader()->restoreState(settings.value("header_state").toByteArray()); 
  settings.endGroup();
}

