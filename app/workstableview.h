#ifndef WORKSTABLEVIEW_H
#define WORKSTABLEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QSortFilterProxyModel>

class DB;

class WorksTableView : public QTableView
{
  Q_OBJECT

  public:
    explicit WorksTableView(QWidget *parent = 0);
    void setDB(DB * db);
    ~WorksTableView();
  signals:
    void workSelected(int workid);
  private:
    DB * mDB = nullptr;
};

class WorksSortFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
  public:
    WorksSortFilterProxyModel(QObject * parent = nullptr);
  protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const;
};

#endif // WORKSTABLEVIEW_H
