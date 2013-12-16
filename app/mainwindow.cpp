#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "db.h"

#include <QSqlQueryModel>

MainWindow::MainWindow(DB *db, QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  mDB(db)
{
  ui->setupUi(this);
  ui->allWorks->setDB(db);
}

MainWindow::~MainWindow()
{
  delete ui;
}
