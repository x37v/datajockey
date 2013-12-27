#ifndef WORKSTABLEVIEW_H
#define WORKSTABLEVIEW_H

#include <QWidget>
#include <QTableView>

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

#endif // WORKSTABLEVIEW_H
