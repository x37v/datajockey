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
#include "db.hpp"
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
#include <QTime>
#include <QPainter>
#include <QStyledItemDelegate>

#include <iostream>
using std::cout;
using std::endl;

class TimeDisplayDelegate : public QStyledItemDelegate {
  public:
    TimeDisplayDelegate(QObject *parent) : QStyledItemDelegate(parent) { }
    virtual ~TimeDisplayDelegate() { }

    virtual QString displayText(const QVariant& value, const QLocale& /* locale */) const {
      int sec = value.toInt();
      int min = sec / 60;
      sec = sec % 60;
      return QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
    }
};

class SessionDisplayDelegate : public QStyledItemDelegate {
  public:
    SessionDisplayDelegate(int current_session, QObject *parent) :
      QStyledItemDelegate(parent), mCurrentSessionId(current_session) {
        mSessionColumn = dj::model::db::work_table_column("session");
      }
    virtual ~SessionDisplayDelegate() { }

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
      QModelIndex session_index = index.sibling(index.row(), mSessionColumn);
      if (session_index.data().toInt() == mCurrentSessionId) {
        painter->setBrush(QBrush(mStyle.background_color()));
        painter->drawRect(option.rect);
      }

      QStyledItemDelegate::paint(painter, option, index);
    }

#if 0
    virtual QString displayText(const QVariant& value, const QLocale& /* locale */) const {
      int session_id = value.toInt();
      if (session_id == mCurrentSessionId)
        return QString("XX");
      else
        return QString("");
    }
#endif
  private:
    int mCurrentSessionId;
    int mSessionColumn;
    SessionDisplayStyle mStyle;
};

WorkDBView::WorkDBView(QAbstractItemModel * model, 
    QWidget *parent) :
  QWidget(parent),
  mWriteSettings(true),
  mLastWork(-1)
{
  //create the layouts
  QVBoxLayout * layout = new QVBoxLayout(this);

  //create and set up the tableview
  mTableView = new QTableView(this);
  mTableView->setSortingEnabled(true);
  mTableView->setModel(model);

  mTableView->setColumnHidden(0, true); //set the id column hidden

  mTableView->horizontalHeader()->setMovable(true);
  //mTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
  mTableView->verticalHeader()->setVisible(false);
  mTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTableView->setSelectionMode(QAbstractItemView::SingleSelection);
  //XXX actually do something with editing at some point
  mTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

  SessionDisplayDelegate * session_delegate = new SessionDisplayDelegate(dj::model::db::work_current_session(), this);
  mTableView->setItemDelegate(session_delegate);

  TimeDisplayDelegate * time_delegate = new TimeDisplayDelegate(this);
  mTableView->setItemDelegateForColumn(dj::model::db::work_table_column("audio_file_seconds"), time_delegate);

  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(1);

  layout->addWidget(mTableView, 10);
  setLayout(layout);

  //connect up our internal signals
  QObject::connect(mTableView->selectionModel(), 
      SIGNAL(selectionChanged(const QItemSelection, const QItemSelection)),
      this,
      SLOT(set_selection(const QItemSelection)));

  QObject::connect(this, SIGNAL(work_selected(int)), SLOT(set_selected_last(int)));

  //schedule the settings reading to happen after view construction
  QTimer::singleShot(0, this, SLOT(read_settings()));
}

QTableView * WorkDBView::tableView(){
  return mTableView;
}

void WorkDBView::select_work(int work_id) {
  //see if we are actually selecting it already
  QModelIndex index = mTableView->selectionModel()->currentIndex(); 
  index = index.sibling(index.row(), dj::model::db::work_table_column("id"));
  if (index.isValid() && mTableView->model()->data(index).toInt() == work_id)
    return; //already selected

  //get the first index
  int rows = mTableView->model()->rowCount();
  //iterate to find our work
  for(int i = 0; i < rows; i++){
    QModelIndex index = mTableView->model()->index(i, dj::model::db::work_table_column("id"));
    QVariant data = index.data();
    if(data.isValid() && data.canConvert(QVariant::Int) && data.toInt() == work_id){
      mTableView->selectRow(index.row());
      emit(work_selected(work_id));
      return;
    }
  }
}

void WorkDBView::select_last() {
  if (mLastWork >= 0)
    select_work(mLastWork);
}

/*
   void WorkDBView::selectWork(const QModelIndex & index ){
   QSqlRecord record = ((QSqlTableModel *)mTableView->model())->record(index.row());
   QVariant itemData = record.value(model::db::work_table_column("id"));
//find the id of the work and emit that
if(itemData.isValid() && itemData.canConvert(QVariant::Int)){
int work = itemData.toInt();
emit(work_selected(work));
}
}
*/

void WorkDBView::set_selection(const QItemSelection & selected) {
  Q_UNUSED(selected);
  QModelIndex index = mTableView->selectionModel()->currentIndex(); 
  index = index.sibling(index.row(), dj::model::db::work_table_column("id"));
  int work_id = -1;
  if(index.isValid())
    work_id = mTableView->model()->data(index).toInt();
  emit(work_selected(work_id));
}

void WorkDBView::set_selected_last(int work_id) {
  if (work_id >= 0)
    mLastWork = work_id;
}

void WorkDBView::write_settings() {
  if (mWriteSettings) {
    QSettings settings;
    settings.beginGroup("WorkDBView");
    settings.setValue("header_state", mTableView->horizontalHeader()->saveState());
    settings.endGroup();
  }
}

void WorkDBView::read_settings() {
  QSettings settings;
  settings.beginGroup("WorkDBView");
  if (settings.contains("header_state"))
    mTableView->horizontalHeader()->restoreState(settings.value("header_state").toByteArray()); 
  settings.endGroup();
}

void WorkDBView::shouldWriteSettings(bool write) { mWriteSettings = write; }

