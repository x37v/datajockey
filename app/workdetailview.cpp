#include "workdetailview.h"
#include "ui_workdetailview.h"
#include "tagmodel.h"
#include <QDragEnterEvent>
#include <QDropEvent>

WorkDetailView::WorkDetailView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WorkDetailView)
{
  ui->setupUi(this);
  setAcceptDrops(true);
}

WorkDetailView::~WorkDetailView() { delete ui; }

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
  ui->tagsView->expandAll();
}

void WorkDetailView::dragEnterEvent(QDragEnterEvent *event) {
  if (mWorkID == 0)
    return;
  if (event->mimeData()->hasFormat(TagModelItemMimeData::format()))
    event->acceptProposedAction();
}

void WorkDetailView::dropEvent(QDropEvent *event) {
  if (mWorkID == 0)
    return;
  if (event->mimeData()->hasFormat(TagModelItemMimeData::format())) {
    const TagModelItemMimeData * data = static_cast<const  TagModelItemMimeData *>(event->mimeData());
    QList<QVariant> ids = data->retrieveData(TagModelItemMimeData::format(), QVariant::List).toList();
    try {
      foreach(QVariant id, ids)
        mDB->work_tag(mWorkID, id.toInt());
      if (mModel) {
        mModel->setWork(mWorkID);
        ui->tagsView->expandAll();
      }
    } catch (std::runtime_error& e) {
      qWarning("problem creating work tag association: %s", e.what());
    }
  }
}

