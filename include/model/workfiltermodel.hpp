#ifndef WORK_FILTER_MODEL_HPP
#define WORK_FILTER_MODEL_HPP

#include <stdexcept>
#include <QSqlQueryModel>
#include <QSortFilterProxyModel>
#include <QDateTime>

class WorkFilterModel : public QSortFilterProxyModel {
  Q_OBJECT

  public:
    WorkFilterModel(QObject * parent = NULL, QSqlDatabase db = QSqlDatabase());

  public slots:
    void set_filter_expression(QString expression);
    void set_current_bpm(double bpm);
    void update_history(int work_id, int session_id, QDateTime played_at);

    //validate a filter expression string
    static bool valid_filter_expression(QString expression);

  signals:
    void filter_expression_changed(QString expression);
    void filter_expression_error(QString expression);
    void sql_changed(QString sql_expression);

  private:
    void apply_filter_expression(QString expression) throw(std::runtime_error);
    QString mFilterExpression;
    QString mSQLExpression;
    QString mTable;
    double mCurrentBPM;
    QSqlQueryModel * mQueryModel;
};

#endif
