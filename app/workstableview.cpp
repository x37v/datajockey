#include "workstableview.h"
#include "db.h"
#include "defines.hpp"
#include <QSqlQueryModel>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QSettings>
#include <QTimer>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QPainter>

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
    SessionDisplayDelegate(int current_session, int session_column, QObject *parent) :
      QStyledItemDelegate(parent), mCurrentSessionId(current_session), mSessionColumn(session_column) {
      }
    virtual ~SessionDisplayDelegate() { }

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
      QModelIndex session_index = index.sibling(index.row(), mSessionColumn);
      QVariant data = session_index.data();
      if (!data.isNull() && data.toInt() == mCurrentSessionId) {
        painter->setBrush(QBrush(mStyle.backgroundColorGet()));
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

WorksTableView::WorksTableView(QWidget *parent) :
  QTableView(parent)
{
  //schedule the settings reading to happen after view construction
  QTimer::singleShot(0, this, SLOT(readSettings()));
}

void WorksTableView::setModel(QAbstractItemModel * model) {
  int seconds_column = DB::work_table_column("audio_file_seconds");
  model->setHeaderData(seconds_column, Qt::Horizontal, "time");
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

  TimeDisplayDelegate * time_delegate = new TimeDisplayDelegate(this);
  if (seconds_column >= 0)
    setItemDelegateForColumn(seconds_column, time_delegate);

  SessionDisplayDelegate * session_delegate =
    new SessionDisplayDelegate(mSessionNumber, DB::work_table_column("session"), this);
  setItemDelegate(session_delegate);
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

void WorksTableView::setSessionNumber(int session) {
  mSessionNumber = session;
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

