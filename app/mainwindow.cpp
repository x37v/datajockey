#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "db.h"
#include "audiomodel.h"
#include "audioloader.h"
#include "mixerpanelview.h"
#include "workfiltermodelcollection.hpp"
#include "workfiltermodel.hpp"
#include "workfilterview.h"
#include "tagmodel.h"
#include "tagsview.h"

#include <QSqlQueryModel>
#include <QToolButton>
#include <QSettings>
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow(DB *db, AudioModel * audio, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  mDB(db)
{
  ui->setupUi(this);
  QSqlQueryModel *model = new QSqlQueryModel(this);
  model->setQuery(db->work_table_query(), db->get());

  centralWidget()->layout()->setContentsMargins(2,2,2,2);

  ui->workDetail->setDB(db);

  //QSortFilterProxyModel * sortable = new WorksSortFilterProxyModel(model);
  QSortFilterProxyModel * sortable = new QSortFilterProxyModel(model);
  sortable->setSourceModel(model);
  sortable->setSortCaseSensitivity(Qt::CaseInsensitive);
  ui->allWorks->setSessionNumber(db->current_session());
  ui->allWorks->setModel(sortable);

  MixerPanelView * mixer = ui->mixer;
  connect(mixer, &MixerPanelView::playerValueChangedDouble, audio, &AudioModel::playerSetValueDouble);
  connect(mixer, &MixerPanelView::playerValueChangedInt,    audio, &AudioModel::playerSetValueInt);
  connect(mixer, &MixerPanelView::playerValueChangedBool,   audio, &AudioModel::playerSetValueBool);
  connect(mixer, &MixerPanelView::playerTriggered,          audio, &AudioModel::playerTrigger);

  connect(mixer, &MixerPanelView::masterValueChangedDouble, audio, &AudioModel::masterSetValueDouble);
  connect(mixer, &MixerPanelView::masterValueChangedInt,    audio, &AudioModel::masterSetValueInt);
  connect(mixer, &MixerPanelView::masterValueChangedBool,   audio, &AudioModel::masterSetValueBool);
  connect(mixer, &MixerPanelView::masterTriggered,          audio, &AudioModel::masterTrigger);

  connect(audio, &AudioModel::playerValueChangedDouble, mixer, &MixerPanelView::playerSetValueDouble);
  connect(audio, &AudioModel::playerValueChangedInt,    mixer, &MixerPanelView::playerSetValueInt);
  connect(audio, &AudioModel::playerValueChangedBool,   mixer, &MixerPanelView::playerSetValueBool);

  connect(audio, &AudioModel::masterValueChangedDouble, mixer, &MixerPanelView::masterSetValueDouble);
  connect(audio, &AudioModel::masterValueChangedInt,    mixer, &MixerPanelView::masterSetValueInt);
  connect(audio, &AudioModel::masterValueChangedBool,   mixer, &MixerPanelView::masterSetValueBool);

  connect(audio, &AudioModel::jumpUpdated, mixer, &MixerPanelView::jumpUpdate);
  connect(audio, &AudioModel::jumpsCleared, mixer, &MixerPanelView::jumpsClear);
  connect(audio, &AudioModel::jumpCleared, mixer, &MixerPanelView::jumpClear);

  connect(ui->allWorks, &WorksTableView::workSelected, this, &MainWindow::selectWork);

  ui->topSplitter->setStretchFactor(0,0);
  ui->topSplitter->setStretchFactor(1,1);
  ui->leftSplitter->setStretchFactor(0,1);
  ui->leftSplitter->setStretchFactor(1,0);

  //remove the close button for the 'all works' tab
  ui->workViews->tabBar()->tabButton(0, QTabBar::RightSide)->resize(0, 0);

  QToolButton * newTabButton = new QToolButton(ui->workViews);
  newTabButton->setText("+");
  ui->workViews->setCornerWidget(newTabButton);

  mFilterCollection = new WorkFilterModelCollection(db, this);
  connect(audio, &AudioModel::masterValueChangedDouble, mFilterCollection, &WorkFilterModelCollection::masterSetValueDouble);
  //XXX do history

  connect(newTabButton, &QToolButton::clicked, [this]() { addFilterTab(); });

  connect(ui->workViews, &QTabWidget::tabCloseRequested, [this] (int index) {
    ui->workViews->removeTab(index);
  });

  //we always want the visible selection to be what gets loaded if we hit load
  connect(ui->workViews, &QTabWidget::currentChanged, [this] (int index) {
    QWidget * tab = ui->workViews->widget(index);
    if (!tab)
      return;
    if (tab == ui->allWorksTab) {
      ui->allWorks->emitSelected();
      return;
    } 
    WorkFilterView * fv = dynamic_cast<WorkFilterView *>(tab);
    if (fv) {
      fv->emitSelected();
      return;
    }
  });
  QTimer::singleShot(0, this, SLOT(readSettings()));

  TagModel * tagmodel = new TagModel(mDB, this);
  tagmodel->showAllTags(true);
  ui->tags->setModel(tagmodel);
  connect(ui->tags, &TagsView::newTagRequested, tagmodel, &TagModel::createTag);
  connect(tagmodel, &TagModel::errorCreatingTag, [this](QString errorString) {
    QMessageBox::warning(this, "Error Creating Tag", errorString);
  });

  connect(ui->tags, &TagsView::tagDeleteRequested, tagmodel, &TagModel::deleteTags);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::loader(AudioLoader * loader) {
  MixerPanelView * mixer = ui->mixer;
  connect(mixer, &MixerPanelView::playerTriggered, loader, &AudioLoader::playerTrigger);
  connect(this, &MainWindow::workSelected, loader, &AudioLoader::selectWork);

  connect(loader, &AudioLoader::playerValueChangedInt, mixer, &MixerPanelView::playerSetValueInt);
  connect(loader, &AudioLoader::playerValueChangedString, mixer, &MixerPanelView::playerSetValueString);

  connect(loader, &AudioLoader::playerBuffersChanged, mixer, &MixerPanelView::playerSetBuffers);
}

void MainWindow::readSettings() {
  QSettings settings;
  settings.beginGroup("WorkFilterModelCollection");
  const int count = settings.beginReadArray("filters");
  for (int i = 0; i < count; i++) {
    settings.setArrayIndex(i);
    QString label("filtered");
    if (settings.contains("label"))
      label = settings.value("label").toString();
    WorkFilterView * view = addFilterTab(settings.value("expression").toString(), label);
    if (settings.contains("guiState"))
      view->restoreState(settings.value("guiState").toMap());
  }
  settings.endArray();
  settings.endGroup();

  settings.beginGroup("WorksTableView");
  if (settings.contains("guiState"))
    ui->allWorks->restoreState(settings.value("guiState").toMap());
  settings.endGroup();

  settings.beginGroup("MainWindow");
  //if (settings.contains("geometry")) {
    //restoreGeometry(settings.value("geometry").toByteArray());
  //}
  //if (settings.contains("windowState"))
    //restoreState(settings.value("windowState").toByteArray(), 0);
  if (settings.contains("topSplitter"))
    ui->topSplitter->restoreState(settings.value("topSplitter").toByteArray());
  if (settings.contains("leftSplitter"))
    ui->leftSplitter->restoreState(settings.value("leftSplitter").toByteArray());
  settings.endGroup();
}

void MainWindow::writeSettings() {
  QSettings settings;

  settings.beginGroup("MainWindow");
  //settings.setValue("geometry", saveGeometry());
  //settings.setValue("windowState", saveState(0));
  settings.setValue("topSplitter", ui->topSplitter->saveState());
  settings.setValue("leftSplitter", ui->leftSplitter->saveState());
  settings.endGroup();

  settings.beginGroup("WorksTableView");
  settings.setValue("guiState", ui->allWorks->saveState());
  settings.endGroup();

  settings.beginGroup("WorkFilterModelCollection");

  int valid_index = 0;
  settings.beginWriteArray("filters");
  for (int i = 0; i < ui->workViews->count(); i++) {
    //make sure it is not the all view and the expression isn't empty
    QWidget * widget = ui->workViews->widget(i);
    WorkFilterView * view = dynamic_cast<WorkFilterView *>(widget);
    if (!view)
      continue;
    QString expression = view->filterExpression();
    if (expression.isEmpty())
      continue;

    settings.setArrayIndex(valid_index);
    valid_index++;
    settings.setValue("expression", expression);
    settings.setValue("label", ui->workViews->tabText(ui->workViews->indexOf(view)));
    settings.setValue("guiState", view->saveState());
  }
  settings.endArray();

  settings.endGroup();
}

void MainWindow::masterSetValueInt(QString name, int v) {
  if (name == "select_work_relative") {
    QWidget * tab = ui->workViews->currentWidget();
    if (!tab)
      return;
    if (tab == ui->allWorksTab) {
      ui->allWorks->selectWorkRelative(v);
      return;
    } 
    WorkFilterView * fv = dynamic_cast<WorkFilterView *>(tab);
    if (fv) {
      fv->selectWorkRelative(v);
      return;
    }
  }
}

void MainWindow::workUpdateHistory(int work_id, QDateTime played_at) {
  mFilterCollection->updateHistory(work_id, played_at);
}

void MainWindow::finalize() {
  ui->workDetail->finalize();
  writeSettings();
}

void MainWindow::selectWork(int id) {
  ui->workDetail->selectWork(id);
  emit(workSelected(id));
}

WorkFilterView * MainWindow::addFilterTab(QString filterExpression, QString title) {
  WorkFilterView * view = new WorkFilterView(this);
  WorkFilterModel * model = mFilterCollection->newFilterModel(view);
  model->setFilterExpression(filterExpression);
  view->setSessionNumber(mDB->current_session());
  view->setModel(model);
  ui->workViews->addTab(view, title);
  connect(view, &WorkFilterView::workSelected, this, &MainWindow::selectWork);

  //make it reflect the first tab or the main view
  QMap<QString, QVariant> state;
  state["tableState"] = ui->allWorks->saveState();

  for (int i = 0; i < ui->workViews->count(); i++) {
    QWidget * widget = ui->workViews->widget(i);
    WorkFilterView * view = dynamic_cast<WorkFilterView *>(widget);
    if (!view)
      continue;
    state = view->saveState();
    break;
  }
  view->restoreState(state);

  return view;
}
