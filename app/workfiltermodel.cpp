#include "workfiltermodel.hpp"
#include "db.h"
#include <stdexcept>
#include <QRegExp>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QtDebug>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

namespace {
  //remove leading & trailing whitespace, and surrounding quotes
  QString cleanup_string(QString input) {
    input.replace(QRegExp("^\\s*"), QString::null);
    input.replace(QRegExp("\\s*$"), QString::null);
    input.replace(QRegExp("^\'(.+)\'$"), "\\1");
    input.replace(QRegExp("^\"(.+)\"$"), "\\1");
    input.replace(QRegExp("^\\s*"), QString::null);
    input.replace(QRegExp("\\s*$"), QString::null);
    return input;
  }

  //(tag foo[,bar[,baz:soda..]])
  const QRegExp tag_reg(
      "\\(tag\\s+((?:[^\\(\\),]+)(?:\\s*,(?:[^\\(\\),]+))*)\\s*\\)",
      Qt::CaseInsensitive);

  //(tag [class:]name) -> (audio_work_tags.tag_id = tag_id)
  //(tag [class1:]name1,[class2:]name2,..) -> (audio_work_tags.tag_id in (tag_id1, tag_id2, tag_id3..))
  QString replace_tag_expressions(QString expression, DB * db) throw(std::runtime_error) {
    QRegExp rx(tag_reg);
    rx.setMinimal(true);

    int pos = 0;
    while ((pos = rx.indexIn(expression, 0)) != -1) {
      const int tag_expr_len = rx.matchedLength();
      QStringList tag_expressions = rx.capturedTexts()[1].split(",");

      //find all the tag ids and fill up the tag_ids list
      QStringList tag_ids;
      foreach (QString tag_expr, tag_expressions) {
        QString tag_name;
        int tag_parent_id = 0;

        //two formats name, or class:name
        QStringList sub_expr = tag_expr.split(":");
        switch (sub_expr.length()) {
          case 1:
            tag_name = cleanup_string(sub_expr[0]);
            break;
          case 2:
            {
              QString tag_parent = cleanup_string(sub_expr[0]);
              tag_name = cleanup_string(sub_expr[1]);
              try {
                tag_parent_id = db->tag_find(tag_parent);
              } catch (std::runtime_error& e) {
                throw std::runtime_error("cannot find tag parent: " + tag_parent.toStdString() + " for expression: " + tag_expr.toStdString());
              }
            }
            break;
          default:
            throw(std::runtime_error("invalid tag expression: " + tag_expr.toStdString()));
        }

        QString tag_id;
        try {
          tag_id.setNum(db->tag_find(tag_name, tag_parent_id));
        } catch (std::runtime_error& e) {
          throw (std::runtime_error("cannot find tag that matches expression: " + tag_expr.toStdString()));
        }
        tag_ids << tag_id;
      }

      //do the replacement
      if (tag_ids.length() == 1) 
        expression.replace(pos, tag_expr_len, "audio_work_tags.tag_id = " + tag_ids[0]);
      else
        expression.replace(pos, tag_expr_len, "audio_work_tags.tag_id IN (" + tag_ids.join(", ") + ")");
    }

    return expression;
  }

  const QRegExp current_bpm_presense("(?:cbpm|current_tempo|current_bpm)", Qt::CaseInsensitive);

  const QRegExp current_bpm_with_mul(
    "\\((?:cbpm|current_tempo|current_bpm)\\s*(\\d+\\.?\\d*%?)\\s*\\)",
      Qt::CaseInsensitive);

  const QRegExp current_bpm_regex(
    "cbpm|current_tempo|current_bpm",
      Qt::CaseInsensitive);

  //(bpm x,y) -> ((bpm >= x) and (bpm <= y)) 
  const QRegExp bpm_range_regex(
    "\\((?:bpm|tempo_median|tempo)\\s*([\\w\\.\\d]+)\\s*,\\s*([\\w\\.\\d]+)\\s*\\)",
      Qt::CaseInsensitive);

  //gt/lt/eq..
  const QRegExp bpm_reg(
      "\\((?:bpm|tempo_median|tempo)\\s*((?:>=|<=|=|>|<)\\s*[\\d\\.]+)\\s*\\)",
      Qt::CaseInsensitive);

  QString replace_tempo_median(QString expression, double current_tempo) throw(std::runtime_error) {
    QRegExp rx;
    int pos = 0;

    //current_bpm_with_mul
    rx = QRegExp(current_bpm_with_mul);
    rx.setMinimal(true);
    while((pos = rx.indexIn(expression, 0)) != -1) {
      const int len = rx.matchedLength();
      QStringList captures = rx.capturedTexts();
      QString mul = captures[1];

      float value = 0;
      if (mul.contains("%")) {
        mul.remove("%");
        value = mul.toFloat() / 100.0f;
      } else {
        value = mul.toFloat();
      }

      QString bottom, top;
      bottom.setNum(current_tempo * (1.0 - value));
      top.setNum(current_tempo * (1.0 + value));

      QString replacement = "((tempo_median >= " + bottom + ") and (tempo_median <= " + top + "))";
      expression.replace(pos, len, replacement);
    }

    //current_bpm
    rx = QRegExp(current_bpm_regex);
    rx.setMinimal(true);
    QString current_tempo_s;
    current_tempo_s.setNum(current_tempo);
    while((pos = rx.indexIn(expression, 0)) != -1) {
      const int len = rx.matchedLength();
      expression.replace(pos, len, current_tempo_s);
    }

    //ranges
    rx = QRegExp(bpm_range_regex);
    rx.setMinimal(true);
    while((pos = rx.indexIn(expression, 0)) != -1) {
      const int len = rx.matchedLength();
      QStringList captures = rx.capturedTexts();
      QString replacement = "((tempo_median >= " + captures[1] + ") and (tempo_median <= " + captures[2] + "))";
      expression.replace(pos, len, replacement);
    }

    //gt/lt..
    rx = QRegExp(bpm_reg);
    rx.setMinimal(true);
    while((pos = rx.indexIn(expression, 0)) != -1) {
      const int len = rx.matchedLength();
      expression.replace(pos, len, "works.tempo_median " + rx.capturedTexts()[1]);
    }

    return expression;
  }

  QString filter_to_sql(DB * db, QString expression, double current_tempo) throw(std::runtime_error) {
    QString original_expr = expression;

    //make sure that there are spaces before and after 'and' and 'or' when they are between parens
    QRegExp and_or_paren("\\)\\s*(and|or)\\s*\\(",
      Qt::CaseInsensitive);
    and_or_paren.setMinimal(true);
    expression.replace(and_or_paren, ") \\1 (");

    expression = replace_tag_expressions(expression, db);
    expression = replace_tempo_median(expression, current_tempo);

    //check parenthesis matching
    if (expression.count('(') != expression.count(')'))
      throw std::runtime_error("unmatched parenthesis in expression: " + original_expr.toStdString());

    return expression;
  }
}

WorkFilterModel::WorkFilterModel(DB * db, QObject * parent) :
  QSortFilterProxyModel(parent),
  mCurrentBPM(120.0),
  mDB(db)
{
  QString query_str = mDB->work_table_query();
  mQueryModel = new QSqlQueryModel(this);
  mQueryModel->setQuery(query_str, mDB->get());
  setSourceModel(mQueryModel);
}

WorkFilterModel::~WorkFilterModel() {
}

void WorkFilterModel::setFilterExpression(QString expression) {
  //XXX just for now, should probably have some sort of 'dirty' flag if we care about
  //current_bpm and other dynamic elements
  //if (mFilterExpression == expression)
    //return;

  mFilterExpression = expression;
  try {
    applyFilterExpression(expression);
    emit(filterExpressionChanged(expression));
    emit(sqlChanged(mSQLExpression));
  } catch (std::runtime_error& e) {
    emit(filterExpressionChanged(expression));
    mSQLExpression = QString();
    emit(sqlChanged(mSQLExpression));
    emit(filterExpressionError(QString::fromStdString(e.what())));
    //cerr << e.what() << endl;
  }
}

void WorkFilterModel::setCurrentBPM(double bpm) {
  mCurrentBPM = bpm;
  if (mFilterExpression.isEmpty() || !mFilterExpression.contains(current_bpm_presense))
    return;

  try {
    applyFilterExpression(mFilterExpression);
  } catch (std::runtime_error& e) {
    //XXX
  }
}

void WorkFilterModel::updateHistory(int /*work_id*/, QDateTime /*played_at*/) {
  if (mFilterExpression.isEmpty())
    return;
  try {
    applyFilterExpression(mFilterExpression);
    //XXX QSqlQueryModel is read only, we can subclass it, make it read write, the we can update it
    //mQueryModel->query();
  } catch (std::runtime_error& e) {
    //XXX
  }
}

bool WorkFilterModel::validFilterExpression(QString /* expression */) {
  //XXX implement!!
  return true;
}

void WorkFilterModel::applyFilterExpression(QString expression) throw(std::runtime_error) {
  QString sql_expr = filter_to_sql(mDB, expression, mCurrentBPM);
  mSQLExpression = sql_expr;

  QString query_str = mDB->work_table_query(mSQLExpression);
  mQueryModel->setQuery(query_str);

  if (mQueryModel->lastError().isValid())
    qDebug() << mQueryModel->lastError();
  emit(applied());
}

