#include "tagsview.h"
#include "ui_tagsview.h"
#include "tagmodel.h"
#include "db.h"
#include <QMessageBox>
#include <QKeyEvent>

TagsView::TagsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagsView)
{
  ui->setupUi(this);
  connect(ui->createButton, &QPushButton::clicked, this, &TagsView::createTag);
}

TagsView::~TagsView()
{
  delete ui;
}

void TagsView::keyPressEvent(QKeyEvent * event) {
	if (event->matches(QKeySequence::Delete) || event->key() == Qt::Key_Backspace) {
    auto indexes = ui->tree->selectionModel()->selectedIndexes();
    if (indexes.size() > 0) {
      emit(tagDeleteRequested(indexes));
    }
  } else
    QWidget::keyPressEvent(event);
}

void TagsView::setModel(QAbstractItemModel * model) {
  ui->tree->setModel(model);
  ui->tree->setHeaderHidden(true);
  ui->tree->setColumnHidden(TagModel::idColumn(), true);
  ui->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ui->tree->setDragEnabled(true);
}

void TagsView::createTag() {
  //get the tag name
  QString tagName = ui->tagName->text().simplified();
  if (tagName.isEmpty()) {
    QMessageBox::warning(this, "Tag empty", "You must provide a name in order to create a tag");
    return;
  }

  QModelIndex parentIndex;
  auto indexes = ui->tree->selectionModel()->selectedIndexes();
  if (indexes.size() > 0)
    parentIndex = indexes.front();
  emit(newTagRequested(tagName, parentIndex));
}

