#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QThread>

#include "db.h"
#include "audiomodel.h"
#include "audioloader.h"
#include "defines.hpp"

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
      [audio](int player, QString name, QString value) {
        if (name == "loading_work")
          audio->playerClear(player);
      });

  MainWindow w(db, audio);
  w.loader(loader);

  QObject::connect(&a, &QApplication::aboutToQuit, [&audio, &w] {
    w.writeSettings();
    audio->run(false);
    QThread::msleep(200);
  });
  w.show();

  return a.exec();
}
