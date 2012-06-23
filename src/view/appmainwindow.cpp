#include "appmainwindow.hpp"

#include <QMainWindow>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFile>

MainWindow::MainWindow(QWidget * parent) : QMainWindow(parent) {
   createActions();
   createMenus();
}

MainWindow::~MainWindow() {
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
   QMenu menu(this);
   //menu.addAction(cutAct);
   //menu.addAction(copyAct);
   //menu.addAction(pasteAct);
   menu.exec(event->globalPos());
}

void MainWindow::createActions() {
   quitAct = new QAction(tr("&quit"), this);
   quitAct->setShortcuts(QKeySequence::Quit);
   quitAct->setStatusTip(tr("Exit the application"));
   connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

   aboutAct = new QAction(tr("&About"), this);
   aboutAct->setStatusTip(tr("Show the application's About box"));
   connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::createMenus() {
   fileMenu = menuBar()->addMenu(tr("&file"));
   fileMenu->addAction(quitAct);

   preferencesMenu = menuBar()->addMenu(tr("&preferences"));

   helpMenu = menuBar()->addMenu(tr("&help"));
   helpMenu->addAction(aboutAct);
}

void MainWindow::about() {
   QFile text(":resources/text/about.html");
   text.open(QFile::ReadOnly);

   QMessageBox::about(this, tr("About DataJockey"), text.readAll());
}

