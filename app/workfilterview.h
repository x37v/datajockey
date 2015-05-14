#ifndef WORKFILTERVIEW_H
#define WORKFILTERVIEW_H

#include <QWidget>
#include <QMap>
#include <QByteArray>
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
    QMap<QString, QVariant> saveState() const;
    bool restoreState(const QMap<QString, QVariant>& state);
  public slots:
    void setSessionNumber(int session);
    void selectWorkRelative(int rows);
    void emitSelected();
    void workUpdateHistory(int work_id, QDateTime played_at);
  signals:
    void workSelected(int workid);

  private:
    Ui::WorkFilterView *ui;
};

#endif // WORKFILTERVIEW_H
