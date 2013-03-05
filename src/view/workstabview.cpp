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
#include <QMessageBox>

using namespace dj;

WorksTabView::WorksTabView(WorkFilterModelCollection * filter_model_collection, QWidget * parent) :
  QWidget(parent),
  mFilterModelCollection(filter_model_collection)
{
  mTabWidget = new RenameableTabWidget(this);

  QSqlTableModel * maintable = new QSqlTableModel(this, model::db::get());
  maintable->setTable("works");
  maintable->select();
  mAllView = new WorkDBView(maintable);
  mTabWidget->addTab(mAllView, "all works");
  mTabWidget->setTabsClosable(true);
  mTabWidget->setMovable(true);

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

  QObject::connect(
      mTabWidget,
      SIGNAL(tabCloseRequested(int)),
      SLOT(close_tab(int)));

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
  table->setParent(view);

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
    QString label;
    if (settings.contains("label"))
      label = settings.value("label").toString();
    create_filter_tab(settings.value("expression").toString(), label);
  }
  settings.endArray();

  settings.endGroup();
}

void WorksTabView::close_tab(int index) {
  if (index < 0 || index >= mTabWidget->count())
    return;
  if (mTabWidget->widget(index) == mAllView)
    return;

  //prompt to be sure
  QMessageBox::StandardButton btn = QMessageBox::question(this,
      "Removing filter",
      QString("Are you sure you want to remove the filter: %1").arg(mTabWidget->tabText(index)),
      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

  if (btn == QMessageBox::Yes) {
    FilteredDBView * view = static_cast<FilteredDBView*>(mTabWidget->widget(index));
    view->deleteLater();
    mTabWidget->close_tab(index);
  }
}

void WorksTabView::write_settings() {
  mAllView->write_settings();
  QSettings settings;
  settings.beginGroup("WorksTabView");

  int valid_index = 0;
  settings.beginWriteArray("filters");
  for (int i = 0; i < mTabWidget->count(); i++) {
    //make sure it is not the all view and the expression isn't empty
    QWidget * widget = mTabWidget->widget(i);
    if (widget == mAllView)
      continue;

    FilteredDBView * view = static_cast<FilteredDBView *>(widget);
    QString expression = view->filter_expression();
    if (expression.isEmpty())
      continue;

    settings.setArrayIndex(valid_index);
    valid_index++;
    settings.setValue("expression", expression);
    settings.setValue("label", mTabWidget->tabText(mTabWidget->indexOf(view)));
  }
  settings.endArray();

  settings.endGroup();
}

