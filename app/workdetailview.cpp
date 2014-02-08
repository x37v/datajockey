#include "workdetailview.h"
#include "ui_workdetailview.h"
#include "tagmodel.h"

WorkDetailView::WorkDetailView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WorkDetailView)
{
  ui->setupUi(this);
}

void WorkDetailView::setDB(DB* db) {
  mDB = db;
  mModel = new TagModel(db, this);
  mModel->showAllTags(false);
  ui->tagsView->setModel(mModel);
}

void WorkDetailView::selectWork(int workid) {
  mWorkID = workid;
  QString songinfo("t:$title\na:$artist\nA:$album");
  mDB->format_string_by_id(mWorkID, songinfo);
  ui->details->setText(songinfo);
  if (mModel)
    mModel->setWork(workid);
}

WorkDetailView::~WorkDetailView()
{
    delete ui;
}
