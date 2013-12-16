#include "workfilterview.h"
#include "ui_workfilterview.h"
#include "db.h"
#include "workstableview.h"

WorkFilterView::WorkFilterView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WorkFilterView)
{
  ui->setupUi(this);
}

void WorkFilterView::setDB(DB * db) {
  ui->worksTable->setDB(db);
}

WorkFilterView::~WorkFilterView()
{
  delete ui;
}
