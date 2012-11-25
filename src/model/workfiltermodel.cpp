#include "workfiltermodel.hpp"
#include "db.hpp"

using namespace dj::model;

WorkFilterModel::WorkFilterModel(QObject * parent, QSqlDatabase db) : WorkRelationModel(db::work::filtered_table(), parent, db) {
}

void WorkFilterModel::set_filter_expression(QString expression) {
  mFilterExpression = expression;
  db::work::filtered_update(tableName(), mFilterExpression);
  select();
}

bool WorkFilterModel::valid_filter_expression(QString expression) {
  //XXX implement!!
  return true;
}

