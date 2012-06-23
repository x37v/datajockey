#include "appmainwindow.hpp"

#include <QMainWindow>
#include <QContextMenuEvent>
#include <QMenu>

MainWindow::MainWindow(QWidget * parent) : QMainWindow(parent) {
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

