#include "workstableview.h"
#include "db.h"
#include <QSqlQueryModel>
#include <QHeaderView>
#include <QSortFilterProxyModel>

#include <iostream>
using namespace std;

WorksTableView::WorksTableView(QWidget *parent) :
  QTableView(parent)
{
}

void WorksTableView::setDB(DB * db) {
  mDB = db;

  QSqlQueryModel *model = new QSqlQueryModel(this);
  model->setQuery(db->work_table_query(), db->get());

  QSortFilterProxyModel * sortable = new QSortFilterProxyModel(model);
  sortable->setSourceModel(model);

  setModel(sortable);

  //hide the id
  setColumnHidden(0, true);
  setSortingEnabled(true);
  horizontalHeader()->setSectionsMovable(true);
  //mTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
  verticalHeader()->setVisible(false);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  //XXX actually do something with editing at some point
  //setEditTriggers(QAbstractItemView::DoubleClicked);
  
  connect(this, &QAbstractItemView::clicked, [this](const QModelIndex& index) {
      int workid = index.sibling(index.row(), 0).data().toInt();
      emit(workSelected(workid));
      });
}

WorksTableView::~WorksTableView()
{
}
