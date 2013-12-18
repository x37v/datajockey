#include "workstableview.h"
#include "db.h"
#include <QSqlQueryModel>
#include <QHeaderView>
#include <QSortFilterProxyModel>

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
}

WorksTableView::~WorksTableView()
{
}
