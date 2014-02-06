#ifndef WORKDETAILVIEW_H
#define WORKDETAILVIEW_H

#include <QWidget>
#include "db.h"

namespace Ui {
  class WorkDetailView;
}

class WorkDetailView : public QWidget
{
  Q_OBJECT

  public:
    explicit WorkDetailView(QWidget *parent = 0);
    void setDB(DB* db);
    ~WorkDetailView();
  public slots:
    void selectWork(int workid);

  private:
    Ui::WorkDetailView *ui;
    DB * mDB;
    int mWorkID = 0;
};

#endif // WORKDETAILVIEW_H
