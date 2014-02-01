#include "workfilterview.h"
#include "ui_workfilterview.h"
#include "db.h"
#include "workstableview.h"

WorkFilterView::WorkFilterView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WorkFilterView)
{
  ui->setupUi(this);
  connect(ui->worksTable, &WorksTableView::workSelected, this, &WorkFilterView::workSelected);
}

void WorkFilterView::setModel(QAbstractItemModel * model) {
  ui->worksTable->setModel(model);
}

WorkFilterView::~WorkFilterView()
{
  delete ui;
}
