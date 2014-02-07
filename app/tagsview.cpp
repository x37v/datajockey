#include "tagsview.h"
#include "ui_tagsview.h"

TagsView::TagsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagsView)
{
    ui->setupUi(this);
}

TagsView::~TagsView()
{
    delete ui;
}

void TagsView::setModel(QAbstractItemModel * model) {
  connect(model, &QAbstractItemModel::modelReset, ui->tree, &QTreeView::reset);
  ui->tree->setModel(model);
  ui->tree->reset();
}
