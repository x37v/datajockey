#include "workstabview.hpp"
#include "workdbview.hpp"
#include "filtereddbview.hpp"
#include "workfiltermodel.hpp"
#include "db.hpp"

#include <QVBoxLayout>
#include <QSqlTableModel>
#include <QToolButton>
#include <QSettings>
#include <QTimer>

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
      SLOT(add_filter()));
  mTabWidget->setCornerWidget(new_btn);

  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->addWidget(mTabWidget);
  setLayout(layout);

  QTimer::singleShot(0, this, SLOT(read_settings()));
}

WorksTabView::~WorksTabView() {
}

void WorksTabView::select_work(int work_id) {
  emit(work_selected(work_id));
}

void WorksTabView::add_filter(QString expression, QString label) {
  create_filter_tab(expression, label);
  mTabWidget->setCurrentIndex(mTabWidget->count() - 1);
}

void WorksTabView::create_filter_tab(QString expression, QString label) {
  WorkFilterModel * table = mFilterModelCollection->new_filter_model();
  FilteredDBView * view = new FilteredDBView(table, this);
  mFilterViews << view;

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

  if (label.isEmpty())
    label = QString("filter %1").arg(mTabWidget->count());
  if (!expression.isEmpty()) {
    view->set_filter_expression(expression);
    view->apply();
  }

  mTabWidget->addTab(view, label);
}

void WorksTabView::read_settings() {
  QSettings settings;
  settings.beginGroup("WorksTabView");

  const int count = settings.beginReadArray("filters");
  for (int i = 0; i < count; i++) {
    settings.setArrayIndex(i);
    create_filter_tab(settings.value("expression").toString());
  }
  settings.endArray();

  settings.endGroup();
}


void WorksTabView::write_settings() {
  mAllView->write_settings();
  QSettings settings;
  settings.beginGroup("WorksTabView");

  int valid_index = 0;
  settings.beginWriteArray("filters");
  foreach (FilteredDBView * view, mFilterViews) {
    QString expression = view->filter_expression();
    if (expression.isEmpty())
      continue;
    settings.setArrayIndex(valid_index);
    valid_index++;
    settings.setValue("expression", expression);
  }
  settings.endArray();

  settings.endGroup();
}

