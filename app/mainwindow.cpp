#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "db.h"
#include "audiomodel.h"
#include "audioloader.h"
#include "mixerpanelview.h"
#include "workfiltermodelcollection.hpp"
#include "workfiltermodel.hpp"
#include "workfilterview.h"

#include <QSqlQueryModel>
#include <QToolButton>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(DB *db, AudioModel * audio, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  mDB(db)
{
  ui->setupUi(this);
  QSqlQueryModel *model = new QSqlQueryModel(this);
  model->setQuery(db->work_table_query(), db->get());

  //QSortFilterProxyModel * sortable = new WorksSortFilterProxyModel(model);
  QSortFilterProxyModel * sortable = new QSortFilterProxyModel(model);
  sortable->setSourceModel(model);
  sortable->setSortCaseSensitivity(Qt::CaseInsensitive);
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

  connect(ui->allWorks, &WorksTableView::workSelected, this, &MainWindow::workSelected);

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
    addFilterTab(settings.value("expression").toString(), label);
  }
  settings.endArray();
  settings.endGroup();
}

void MainWindow::writeSettings() {
  ui->allWorks->writeSettings();

  QSettings settings;

  //settings.setValue("geometry", saveGeometry());

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

void MainWindow::addFilterTab(QString filterExpression, QString title) {
  WorkFilterView * view = new WorkFilterView(this);
  WorkFilterModel * model = mFilterCollection->newFilterModel(view);
  model->setFilterExpression(filterExpression);
  view->setModel(model);
  ui->workViews->addTab(view, title);
  connect(view, &WorkFilterView::workSelected, this, &MainWindow::workSelected);
}
