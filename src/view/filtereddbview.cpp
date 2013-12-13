#include "filtereddbview.hpp"
#include "workdbview.hpp"
#include <QAbstractItemModel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

FilteredDBView::FilteredDBView(dj::model::DB * db, QAbstractItemModel * model, QWidget *parent) : QWidget(parent) {
  //create the layouts
  QVBoxLayout * layout = new QVBoxLayout(this);

  mFilterEditor = new QTextEdit(this);
  mFilterEditor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
  
  QPushButton * submit_button = new QPushButton("apply filter", this);

  mDBView = new WorkDBView(db, model, this);
  mDBView->shouldWriteSettings(false);

  layout->addWidget(mFilterEditor, 1);
  layout->addWidget(submit_button, 0);
  layout->addWidget(mDBView, 20);

  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(1);
  setLayout(layout);

  QObject::connect(mDBView, SIGNAL(work_selected(int)), SIGNAL(work_selected(int)));
  QObject::connect(submit_button, SIGNAL(pressed()), SLOT(apply()));
}

QString FilteredDBView::filter_expression() const { return mFilterEditor->toPlainText(); }
int FilteredDBView::last_selected() const { return mDBView->last_selected(); }

void FilteredDBView::select_work(int work_id) { mDBView->select_work(work_id); }
void FilteredDBView::select_last() { mDBView->select_last(); }

void FilteredDBView::set_filter_expression(QString expression) {
  mFilterEditor->setPlainText(expression);
}

void FilteredDBView::filter_expression_error(QString expression) {
  QMessageBox::warning(this,
      "invalid filter",
      "invalid filter expression: " + expression);
}

void FilteredDBView::apply() {
  emit(filter_expression_changed(mFilterEditor->toPlainText()));
}

