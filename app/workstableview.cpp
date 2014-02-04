#include "workstableview.h"
#include "db.h"
#include "defines.hpp"
#include <QSqlQueryModel>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QTimer>

#include <iostream>
using namespace std;

WorksTableView::WorksTableView(QWidget *parent) :
  QTableView(parent)
{
  //schedule the settings reading to happen after view construction
  QTimer::singleShot(0, this, SLOT(readSettings()));
}

void WorksTableView::setModel(QAbstractItemModel * model) {
  QTableView::setModel(model);

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

WorksTableView::~WorksTableView() { }

void WorksTableView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
  QTableView::selectionChanged(selected, deselected);
  auto indexes = selected.indexes();
  if (indexes.size() == 0)
    return;
  QModelIndex index = indexes.front();
  int workid = index.sibling(index.row(), 0).data().toInt();
  emit(workSelected(workid));
}

void WorksTableView::readSettings() {
  QSettings settings;
  settings.beginGroup("WorksTableView");
  if (settings.contains("headerState"))
    horizontalHeader()->restoreState(settings.value("headerState").toByteArray()); 
  settings.endGroup();
}

void WorksTableView::writeSettings() {
  QSettings settings;
  settings.beginGroup("WorksTableView");
  settings.setValue("headerState", horizontalHeader()->saveState());
  settings.endGroup();
}

void WorksTableView::selectWorkRelative(int rows) {
  int row = 0;
  auto indexes = selectedIndexes();
  if (indexes.size() > 0)
    row = dj::clamp(indexes.front().row() + rows, 0, model()->rowCount() - 1);
  selectRow(row);
}

void WorksTableView::emitSelected() {
  auto indexes = selectedIndexes();
  if (indexes.size() == 0)
    return;
  QModelIndex index = indexes.front();
  int workid = index.sibling(index.row(), 0).data().toInt();
  emit(workSelected(workid));
}

WorksSortFilterProxyModel::WorksSortFilterProxyModel(QObject * parent) : QSortFilterProxyModel(parent)
{
}

bool WorksSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  QVariant leftData = sourceModel()->data(left);
  QVariant rightData = sourceModel()->data(right);
  const int row = left.row();
  const int column = left.column();
  switch (column) {
    case DB::WORK_ARTIST_NAME:
      {
        QString leftArtist = leftData.toString();
        QString rightArtist = rightData.toString();
        if (leftArtist != rightArtist)
          return QString::localeAwareCompare(leftArtist, rightArtist) < 0;
      }
        //intentional drop through
    case DB::WORK_ALBUM_NAME:
      {
        QString leftAlbum = sourceModel()->data(left.sibling(row, DB::WORK_ALBUM_NAME)).toString();
        QString rightAlbum = sourceModel()->data(right.sibling(row, DB::WORK_ALBUM_NAME)).toString();
        if (leftAlbum != rightAlbum)
          return QString::localeAwareCompare(leftAlbum, rightAlbum) < 0;
      }
    case DB::WORK_ALBUM_TRACK:
      {
        int leftTrack = sourceModel()->data(left.sibling(row, DB::WORK_ALBUM_TRACK)).toInt();
        int rightTrack = sourceModel()->data(right.sibling(row, DB::WORK_ALBUM_TRACK)).toInt();
        if (leftTrack != rightTrack)
          return leftTrack < rightTrack;
        QString leftName = sourceModel()->data(left.sibling(row, DB::WORK_NAME)).toString();
        QString rightName = sourceModel()->data(right.sibling(row, DB::WORK_NAME)).toString();
        return QString::localeAwareCompare(leftName, rightName) < 0;
      }
      break;
      //work name is fine, just alphabetical
    case DB::WORK_NAME:
    default:
      break;
  }
  return QSortFilterProxyModel::lessThan(left, right);
}

