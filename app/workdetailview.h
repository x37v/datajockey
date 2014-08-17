#ifndef WORKDETAILVIEW_H
#define WORKDETAILVIEW_H

#include <QWidget>
#include "db.h"

namespace Ui {
  class WorkDetailView;
}

class TagModel;
class WorkDetailView : public QWidget
{
  Q_OBJECT

  public:
    explicit WorkDetailView(QWidget *parent = 0);
    void setDB(DB* db);
    ~WorkDetailView();
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void keyPressEvent(QKeyEvent * event);
  public slots:
    void selectWork(int workid);
    void finalize();

  private:
    void rereadTags();
    Ui::WorkDetailView *ui;
    DB * mDB;
    int mWorkID = 0;
    TagModel * mModel = nullptr;
};

#endif // WORKDETAILVIEW_H
