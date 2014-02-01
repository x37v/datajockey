#include "workfilterview.h"
#include "ui_workfilterview.h"
#include "db.h"
#include "workstableview.h"
#include <QMessageBox>

WorkFilterView::WorkFilterView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WorkFilterView)
{
  ui->setupUi(this);
  connect(ui->worksTable, &WorksTableView::workSelected, this, &WorkFilterView::workSelected);

  ui->splitter->setStretchFactor(0,0);
  ui->splitter->setStretchFactor(1,10);
}

void WorkFilterView::setModel(WorkFilterModel * model) {
  ui->worksTable->setModel(model);
  connect(ui->applyButton, &QPushButton::clicked, [this, model]() {
    model->setFilterExpression(ui->filterEdit->toPlainText());
  });

  connect(model, &WorkFilterModel::filterExpressionError, [this] (QString message) {
    QMessageBox::warning(this,
        "invalid filter",
        "invalid filter expression: " + message);
  });
}

WorkFilterView::~WorkFilterView()
{
  delete ui;
}
