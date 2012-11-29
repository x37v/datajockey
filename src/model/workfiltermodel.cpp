#include "workfiltermodel.hpp"
#include "db.hpp"
#include <stdexcept>
#include <QRegExp>
#include <QStringList>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

using namespace dj::model;

namespace {
  QString remove_quotes(QString input) {
    return input.remove(QRegExp("^(?:\"|\')")).remove(QRegExp("(?:\"|\')$"));
  }

  const QRegExp tag_reg(
      "\\(tag\\s+((?:\"[^\"]*\")|(?:\'[^\']*\')|\\w+)\\s*(?:,\\s*((?:\"[^\"]*\")|(?:\'[^\']*\')|\\w+)){0,1}\\s*\\)",
      Qt::CaseInsensitive);

  //(tag name[,class]) -> (audio_work_tags.tag_id = get_id)
  QString replace_tag_expressions(QString expression) throw(std::runtime_error) {
    QRegExp rx(tag_reg);
    rx.setMinimal(true);

    int pos = 0;
    while((pos = rx.indexIn(expression, 0)) != -1) {
      QString tag_name;
      const int len = rx.matchedLength();
      const int capture_count = rx.captureCount();
      int tag_class_id = -1;
      QStringList captures = rx.capturedTexts();

      //grab name and class id
      tag_name = remove_quotes(captures[1]);
      if (capture_count == 2 && !captures[2].isEmpty()) {
        QString tag_class = remove_quotes(captures[2]);
        tag_class_id = db::tag::find_class(tag_class);
      }

      QString tag_id;
      tag_id.setNum(db::tag::find(tag_name, tag_class_id));
      expression.replace(pos, len, "audio_work_tags.tag_id = " + tag_id);
    }

    return expression;
  }

  const QRegExp bpm_reg(
      "\\((?:bpm|tempo_median|tempo)\\s*((?:>=|<=|=|>|<)\\s*[\\d\\.]+)\\s*\\)",
      Qt::CaseInsensitive);

  QString replace_tempo_median(QString expression) throw(std::runtime_error) {
    QRegExp rx(bpm_reg);
    rx.setMinimal(true);
    int pos = 0;
    while((pos = rx.indexIn(expression, 0)) != -1) {
      const int len = rx.matchedLength();
      expression.replace(pos, len, "works.tempo_median " + rx.capturedTexts()[1]);
    }
    return expression;
  }

  QString filter_to_sql(QString expression) throw(std::runtime_error) {
    expression = replace_tag_expressions(expression);
    expression = replace_tempo_median(expression);
    return expression;
  }
}

WorkFilterModel::WorkFilterModel(QObject * parent, QSqlDatabase db) : WorkRelationModel(db::work::filtered_table(), parent, db) {
}

void WorkFilterModel::set_filter_expression(QString expression) {
  if (mFilterExpression == expression)
    return;

  mFilterExpression = expression;
  try {
    QString sql_expr = filter_to_sql(expression);
    //cout << sql_expr.toStdString() << endl;
    db::work::filtered_update(tableName(), sql_expr);
    select();

    emit(filter_expression_changed(expression));
    emit(sql_changed(sql_expr));
  } catch (std::runtime_error& e) {
    emit(filter_expression_changed(expression));
    emit(sql_changed(QString()));
    emit(filter_expression_error(QString::fromStdString(e.what())));
    //cerr << e.what() << endl;
  }
}

bool WorkFilterModel::valid_filter_expression(QString /* expression */) {
  //XXX implement!!
  return true;
}

