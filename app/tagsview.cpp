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
