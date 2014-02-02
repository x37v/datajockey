#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QThread>
#include <QTimer>
#include <QErrorMessage>

#include "db.h"
#include "audiomodel.h"
#include "audioloader.h"
#include "defines.hpp"
#include "midirouter.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QApplication::setStyle(QStyleFactory::create("Fusion"));

  a.setApplicationVersion(dj::version_string);
  a.setApplicationName("Data Jockey " + a.applicationVersion());

  //for global qsettings
  a.setOrganizationName("xnor");
  a.setOrganizationDomain("x37v.info");
  a.setApplicationName("DataJockey");

  QPalette palette;
  palette.setColor(QPalette::Window, QColor(53,53,53));
  palette.setColor(QPalette::WindowText, Qt::white);
  palette.setColor(QPalette::Base, QColor(15,15,15));
  palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
  palette.setColor(QPalette::ToolTipBase, Qt::white);
  palette.setColor(QPalette::ToolTipText, Qt::white);
  palette.setColor(QPalette::Text, Qt::white);
  palette.setColor(QPalette::Button, QColor(53,53,53));
  palette.setColor(QPalette::ButtonText, Qt::white);
  palette.setColor(QPalette::BrightText, Qt::red);

  palette.setColor(QPalette::Highlight, QColor(142,45,197).lighter());
  palette.setColor(QPalette::HighlightedText, Qt::black);
  a.setPalette(palette);

  DB * db = new DB("QSQLITE", "/home/alex/.datajockey/database.sqlite3");
  AudioModel * audio = new AudioModel();
  audio->run(true);

  AudioLoader * loader = new AudioLoader(db, audio);
  QObject::connect(loader, &AudioLoader::playerBuffersChanged, audio, &AudioModel::playerLoad);
  QObject::connect(loader, &AudioLoader::playerValueChangedString,
      [audio](int player, QString name, QString /*value*/) {
        if (name == "loading_work")
          audio->playerClear(player);
      });

  MidiRouter * midi = new MidiRouter(audio->audioio()->midi_input_ringbuffer());
  QThread * midiThread = new QThread;
  midi->moveToThread(midiThread);
  QTimer * midiProcessTimer = new QTimer();
  midiProcessTimer->setInterval(15);
  QObject::connect(midiProcessTimer, &QTimer::timeout, midi, &MidiRouter::process);
  QObject::connect(midiThread, &QThread::finished, midiProcessTimer, &QTimer::stop);
  midiThread->start(QThread::HighPriority);
  midiProcessTimer->start();

  QObject::connect(midi, &MidiRouter::playerValueChangedDouble, audio, &AudioModel::playerSetValueDouble);
  QObject::connect(midi, &MidiRouter::playerValueChangedInt,    audio, &AudioModel::playerSetValueInt);
  QObject::connect(midi, &MidiRouter::playerValueChangedBool,   audio, &AudioModel::playerSetValueBool);
  QObject::connect(midi, &MidiRouter::playerTriggered,          audio, &AudioModel::playerTrigger);

  QObject::connect(midi, &MidiRouter::masterValueChangedDouble, audio, &AudioModel::masterSetValueDouble);
  QObject::connect(midi, &MidiRouter::masterValueChangedInt,    audio, &AudioModel::masterSetValueInt);
  QObject::connect(midi, &MidiRouter::masterValueChangedBool,   audio, &AudioModel::masterSetValueBool);
  QObject::connect(midi, &MidiRouter::masterTriggered,          audio, &AudioModel::masterTrigger);

  QErrorMessage * midiErrors = new QErrorMessage;
  QObject::connect(midi, &MidiRouter::mappingError, midiErrors, static_cast<void (QErrorMessage::*)(const QString&)>(&QErrorMessage::showMessage));

  midi->readFile("./midimapping.yaml");

  MainWindow w(db, audio);
  w.loader(loader);

  QObject::connect(&a, &QApplication::aboutToQuit, [&audio, &w, &midiThread] {
    w.writeSettings();
    midiThread->quit();
    audio->run(false);
    QThread::msleep(200);
  });
  w.show();

  return a.exec();
}
