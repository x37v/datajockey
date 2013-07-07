#ifndef WORK_FILTER_MODEL_HPP
#define WORK_FILTER_MODEL_HPP

#include <stdexcept>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include "db.hpp"

class WorkFilterModel : public QSortFilterProxyModel {
  Q_OBJECT

  public:
    WorkFilterModel(dj::model::DB * db, QObject * parent = NULL);
    virtual ~WorkFilterModel();

  public slots:
    void set_filter_expression(QString expression);
    void set_current_bpm(double bpm);
    void update_history(int work_id, QDateTime played_at);

    //validate a filter expression string
    static bool valid_filter_expression(QString expression);

  signals:
    void filter_expression_changed(QString expression);
    void filter_expression_error(QString expression);
    void sql_changed(QString sql_expression);
    void applied();

  private:
    void apply_filter_expression(QString expression) throw(std::runtime_error);
    QString mFilterExpression;
    QString mSQLExpression;
    QString mTable;
    double mCurrentBPM;
    QSqlQueryModel * mQueryModel;
    dj::model::DB * mDB;
};

#endif
