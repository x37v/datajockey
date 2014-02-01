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

  QToolButton * newTabButton = new QToolButton(ui->workViews);
  newTabButton->setText("+");
  ui->workViews->setCornerWidget(newTabButton);

  WorkFilterModelCollection * filterCollection = new WorkFilterModelCollection(db, this);
  connect(audio, &AudioModel::masterValueChangedDouble, filterCollection, &WorkFilterModelCollection::masterSetValueDouble);
  //XXX do history

  connect(newTabButton, &QToolButton::clicked, [this, filterCollection]() {
    WorkFilterView * view = new WorkFilterView(this);
    WorkFilterModel * model = filterCollection->newFilterModel(view);
    view->setModel(model);
    ui->workViews->addTab(view, "filtered");
    connect(view, &WorkFilterView::workSelected, this, &MainWindow::workSelected);
  });

  connect(ui->workViews, &QTabWidget::tabCloseRequested, [this] (int index) {
    if (index == 0) //don't allow the first tab to be closed
      return;
    ui->workViews->removeTab(index);
  });
}

void MainWindow::loader(AudioLoader * loader) {
  MixerPanelView * mixer = ui->mixer;
  connect(mixer, &MixerPanelView::playerTriggered, loader, &AudioLoader::playerTrigger);
  connect(this, &MainWindow::workSelected, loader, &AudioLoader::selectWork);

  connect(loader, &AudioLoader::playerValueChangedInt, mixer, &MixerPanelView::playerSetValueInt);
  connect(loader, &AudioLoader::playerValueChangedString, mixer, &MixerPanelView::playerSetValueString);

  connect(loader, &AudioLoader::playerBuffersChanged, mixer, &MixerPanelView::playerSetBuffers);
}

MainWindow::~MainWindow()
{
  delete ui;
}
