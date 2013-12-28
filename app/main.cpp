#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include "db.h"
#include "audiomodel.h"
#include "audioloader.h"

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QApplication::setStyle(QStyleFactory::create("Fusion"));

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
  QObject::connect(loader, &AudioLoader::playerLoaded, audio, &AudioModel::playerLoad);
  QObject::connect(loader, &AudioLoader::playerLoadingInfo,
      [audio](int player, QString info) {
        audio->playerClear(player);
      });

  MainWindow w(db, audio);
  w.loader(loader);
  w.show();

  return a.exec();
}
