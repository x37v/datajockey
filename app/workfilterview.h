#ifndef WORKFILTERVIEW_H
#define WORKFILTERVIEW_H

#include <QWidget>

namespace Ui {
  class WorkFilterView;
}

class DB;

class WorkFilterView : public QWidget
{
  Q_OBJECT

  public:
    explicit WorkFilterView(QWidget *parent = 0);
    ~WorkFilterView();
    void setDB(DB * db);

  private:
    Ui::WorkFilterView *ui;
};

#endif // WORKFILTERVIEW_H
