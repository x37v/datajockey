#include "workdetailview.h"
#include "ui_workdetailview.h"

WorkDetailView::WorkDetailView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WorkDetailView)
{
    ui->setupUi(this);
}

WorkDetailView::~WorkDetailView()
{
    delete ui;
}
