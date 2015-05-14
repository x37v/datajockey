#include "workfilterview.h"
#include "ui_workfilterview.h"
#include "db.h"
#include "workstableview.h"
#include <QMessageBox>
#include <QMap>
#include <QDataStream>

WorkFilterView::WorkFilterView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::WorkFilterView)
{
  ui->setupUi(this);
  connect(ui->worksTable, &WorksTableView::workSelected, this, &WorkFilterView::workSelected);
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

QMap<QString, QVariant> WorkFilterView::saveState() const {
  QMap<QString, QVariant> state;
  state["splitter"] = ui->splitter->saveState();
  return state;
}

bool WorkFilterView::restoreState(const QMap<QString, QVariant>& state) {
  auto it = state.find("splitter");
  if (it != state.end())
    ui->splitter->restoreState(it->toByteArray());
  return true;
}

void WorkFilterView::setSessionNumber(int session) { ui->worksTable->setSessionNumber(session); }
void WorkFilterView::selectWorkRelative(int rows) { ui->worksTable->selectWorkRelative(rows); }
void WorkFilterView::emitSelected() { ui->worksTable->emitSelected(); }
void WorkFilterView::workUpdateHistory(int work_id, QDateTime played_at) { ui->worksTable->workUpdateHistory(work_id, played_at); }
