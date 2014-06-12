#ifndef WORK_FILTER_MODEL_HPP
#define WORK_FILTER_MODEL_HPP

#include <stdexcept>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include "db.h"

class WorkFilterModel : public QSortFilterProxyModel {
  Q_OBJECT

  public:
    WorkFilterModel(DB * db, QObject * parent = NULL);
    virtual ~WorkFilterModel();

    QString filterExpression() const { return mFilterExpression; }

  public slots:
    void setFilterExpression(QString expression);
    void setCurrentBPM(double bpm);
    void updateHistory(int work_id, QDateTime played_at);

    //validate a filter expression string
    static bool validFilterExpression(QString expression);

  signals:
    void filterExpressionChanged(QString expression);
    void filterExpressionError(QString expression);
    void sqlChanged(QString sql_expression);
    void applied();
 
  private:
    void applyFilterExpression(QString expression) throw(std::runtime_error);
    QString mFilterExpression;
    QString mSQLExpression;
    QString mTable;
    double mCurrentBPM;
    QSqlQueryModel * mQueryModel;
    DB * mDB;
};

#endif
