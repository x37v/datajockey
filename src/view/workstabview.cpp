#include "workstabview.hpp"
#include "workdbview.hpp"
#include "filtereddbview.hpp"
#include "workfiltermodel.hpp"
#include "db.hpp"

#include <QVBoxLayout>
#include <QSqlTableModel>
#include <QToolButton>

using namespace dj;

WorksTabView::WorksTabView(WorkFilterModelCollection * filter_model_collection, QWidget * parent) :
  QWidget(parent),
  mFilterModelCollection(filter_model_collection)
{
  mTabWidget = new QTabWidget();

  QSqlTableModel * maintable = new QSqlTableModel(this, model::db::get());
  maintable->setTable("works");
  maintable->select();
  mAllView = new WorkDBView(maintable);
  mTabWidget->addTab(mAllView, "all works");

  QObject::connect(
      mAllView,
      SIGNAL(work_selected(int)),
      SLOT(select_work(int)));
  QObject::connect(
      this,
      SIGNAL(work_selected(int)),
      mAllView,
      SLOT(select_work(int)));

  //set up the new tab button
  QToolButton * new_btn = new QToolButton(this);
  new_btn->setCursor(Qt::ArrowCursor);
  QObject::connect(
      new_btn,
      SIGNAL(clicked()),
      SLOT(new_tab()));
  mTabWidget->setCornerWidget(new_btn);

  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->addWidget(mTabWidget);
  setLayout(layout);
}

WorksTabView::~WorksTabView() {
}

void WorksTabView::read_settings() {
}

void WorksTabView::select_work(int work_id) {
  emit(work_selected(work_id));
}

void WorksTabView::new_tab() {
  WorkFilterModel * table = mFilterModelCollection->new_filter_model();
  FilteredDBView * view = new FilteredDBView(table, this);

  QObject::connect(
      view,
      SIGNAL(work_selected(int)),
      SLOT(select_work(int)));

  QObject::connect(
      view,
      SIGNAL(filter_expression_changed(QString)),
      table,
      SLOT(set_filter_expression(QString)));
  QObject::connect(
      table,
      SIGNAL(filter_expression_changed(QString)),
      view,
      SLOT(set_filter_expression(QString)));
  QObject::connect(
      table,
      SIGNAL(filter_expression_error(QString)),
      view,
      SLOT(filter_expression_error(QString)));

  mTabWidget->addTab(view, "filtered view");
  mTabWidget->setCurrentIndex(mTabWidget->count() - 1);
}

void WorksTabView::write_settings() {
}

