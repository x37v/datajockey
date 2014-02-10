#include "workdetailview.h"
#include "ui_workdetailview.h"
#include "tagmodel.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>

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
  ui->tagsView->setColumnHidden(TagModel::idColumn(), true);
  ui->details->setTextFormat(Qt::RichText);
}

void WorkDetailView::selectWork(int workid) {
  mWorkID = workid;
  QString songinfo("<div><b>Title:</b> $title</div> <div><b>Artist:</b> $artist</div> <div><b>Album:</b> $album</div>");
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
      rereadTags();
    } catch (std::runtime_error& e) {
      qWarning("problem creating work tag association: %s", e.what());
    }
  }
}

void WorkDetailView::keyPressEvent(QKeyEvent * event) {
	if (event->matches(QKeySequence::Delete) || event->key() == Qt::Key_Backspace) {
    if (mWorkID == 0)
      return;
    QModelIndex index = ui->tagsView->currentIndex();
    if (!(index.isValid() && mModel->canDelete(index)))
      return;
    try {
      Tag * tag = static_cast<Tag *>(index.internalPointer());
      if (!tag)
        return;
      mDB->work_tag_remove(mWorkID, tag->id());
      rereadTags();
    } catch (std::runtime_error& e) {
      qWarning("problem removing work tag association: %s", e.what());
    }
  } else
    QWidget::keyPressEvent(event);
}

void WorkDetailView::rereadTags() {
  if (!mModel)
    return;
  mModel->setWork(mWorkID);
  ui->tagsView->expandAll();
}
