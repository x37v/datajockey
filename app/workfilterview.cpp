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

void WorkFilterView::setModel(WorkFilterModel * model) {
  ui->worksTable->setModel(model);
  connect(ui->applyButton, &QPushButton::clicked, [this, model]() {
    model->setFilterExpression(ui->filterEdit->toPlainText());
  });
}

WorkFilterView::~WorkFilterView()
{
  delete ui;
}
