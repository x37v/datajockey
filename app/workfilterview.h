#ifndef WORKFILTERVIEW_H
#define WORKFILTERVIEW_H

#include <QWidget>
#include "workfiltermodel.hpp"

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
    void setModel(WorkFilterModel * model);
    QString filterExpression() const;
  signals:
    void workSelected(int workid);

  private:
    Ui::WorkFilterView *ui;
};

#endif // WORKFILTERVIEW_H
