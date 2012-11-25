#ifndef WORK_FILTER_MODEL_HPP
#define WORK_FILTER_MODEL_HPP

#include "workrelationmodel.hpp"

class WorkFilterModel : public WorkRelationModel {
  Q_OBJECT

  public:
    WorkFilterModel(QObject * parent = NULL, QSqlDatabase db = QSqlDatabase());

  public slots:
    void set_filter_expression(QString expression);

    //validate a filter expression string
    static bool valid_filter_expression(QString expression);

  private:
    QString mFilterExpression;
    QString mTable;
};

#endif
