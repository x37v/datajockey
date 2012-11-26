#include "filtereddbview.hpp"
#include "workdbview.hpp"
#include <QAbstractItemModel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSplitter>

FilteredDBView::FilteredDBView(QAbstractItemModel * model, QWidget *parent) : QWidget(parent) {
  //create the layouts
  QVBoxLayout * layout = new QVBoxLayout(this);

  mFilterEditor = new QTextEdit(this);

  mDBView = new WorkDBView(model, this);
  mDBView->shouldWriteSettings(false);

  QSplitter * splitter = new QSplitter(Qt::Vertical, this);
  splitter->addWidget(mFilterEditor);
  splitter->addWidget(mDBView);

  layout->addWidget(splitter);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(1);
  setLayout(layout);

  QObject::connect(mDBView, SIGNAL(work_selected(int)), SIGNAL(work_selected(int)));
}

void FilteredDBView::select_work(int work_id) { mDBView->select_work(work_id); }

