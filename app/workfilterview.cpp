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
  ui->filterEdit->setPlainText(model->filterExpression());

  connect(ui->applyButton, &QPushButton::clicked, [this, model]() {
    model->setFilterExpression(ui->filterEdit->toPlainText());
  });

  connect(model, &WorkFilterModel::filterExpressionError, [this] (QString message) {
    QMessageBox::warning(this,
        "invalid filter",
        "invalid filter expression: " + message);
  });

  connect(model, &WorkFilterModel::filterExpressionChanged, [this] (QString expression) {
    ui->filterEdit->setPlainText(expression);
  });
}

QString WorkFilterView::filterExpression() const { return ui->filterEdit->toPlainText(); }
WorkFilterView::~WorkFilterView() { delete ui; }

void WorkFilterView::setSessionNumber(int session) { ui->worksTable->setSessionNumber(session); }
void WorkFilterView::selectWorkRelative(int rows) { ui->worksTable->selectWorkRelative(rows); }
void WorkFilterView::emitSelected() { ui->worksTable->emitSelected(); }
void WorkFilterView::workUpdateHistory(int work_id, QDateTime played_at) { ui->worksTable->workUpdateHistory(work_id, played_at); }