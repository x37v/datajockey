#include "appmainwindow.hpp"
#include "defines.hpp"

#include <QMainWindow>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QFile>

MainWindow::MainWindow(QWidget * parent) : QMainWindow(parent) {
   mMIDIMapper = new dj::view::MIDIMapper(this, Qt::Dialog);
   mMIDIMapper->setVisible(false);

   createActions();
   createMenus();
}

MainWindow::~MainWindow() { }

dj::view::MIDIMapper * MainWindow::midi_mapper() { return mMIDIMapper; }

void MainWindow::contextMenuEvent(QContextMenuEvent *event) {
   QMenu menu(this);
   //menu.addAction(cutAct);
   //menu.addAction(copyAct);
   //menu.addAction(pasteAct);
   menu.exec(event->globalPos());
}

void MainWindow::createActions() {
   quitAct = new QAction(tr("quit"), this);
   quitAct->setShortcuts(QKeySequence::Quit);
   quitAct->setStatusTip(tr("Exit the application"));
   connect(quitAct, SIGNAL(triggered()), this, SLOT(close()));

   aboutAct = new QAction(tr("about"), this);
   aboutAct->setStatusTip(tr("Show the application's About box"));
   connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

   midiMappingAct = new QAction(tr("midi mapping"), this);
   midiMappingAct->setStatusTip(tr("Set up midi mapping"));
   connect(midiMappingAct, SIGNAL(triggered()), this, SLOT(midi_mapping()));
}

void MainWindow::createMenus() {
   fileMenu = menuBar()->addMenu(tr("&file"));
   fileMenu->addAction(quitAct);

   preferencesMenu = menuBar()->addMenu(tr("&preferences"));
   preferencesMenu->addAction(midiMappingAct);

   helpMenu = menuBar()->addMenu(tr("&help"));
   helpMenu->addAction(aboutAct);
}

void MainWindow::about() {
   QFile text(":resources/text/about.html");
   text.open(QFile::ReadOnly);

   QMessageBox::about(this,
         tr("About DataJockey"), 
         "<p>This is Data Jockey version " + dj::version_string + "</p>" + text.readAll());
}

void MainWindow::midi_mapping() {
   mMIDIMapper->setVisible(true);
}
