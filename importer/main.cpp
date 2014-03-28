#include <QCoreApplication>
#include <QCommandLineParser>
#include "fileprocessor.h"
#include "config.hpp"
#include "db.h"
#include "defines.hpp"
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QSet>
#include <QtDebug>

//grabbed from stack overflow
//http://stackoverflow.com/questions/8052460/recursive-scanning-of-directories
void scanDir(QDir dir, QSet<QString>& files) {
  dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

  foreach(QString file, dir.entryList()) {
    QFileInfo fileInfo(dir.filePath(file));
    if (!fileInfo.isFile())
      continue;
    QString ext = fileInfo.suffix();
    if (dj::audio_file_extensions.contains(ext, Qt::CaseInsensitive))
      files.insert(fileInfo.canonicalFilePath());
  }

  dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
  QStringList dirList = dir.entryList();
  for (int i= 0; i < dirList.size(); ++i) {
    QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
    scanDir(QDir(newPath), files);
  }
}

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QCoreApplication::setApplicationName("datajockey_importer");
  QCoreApplication::setApplicationVersion("1.0git"); //XXX deal with it
  QCoreApplication::setOrganizationName("xnor");
  QCoreApplication::setOrganizationDomain("x37v.info");

  QCommandLineOption daemonOption(QStringList() << "d" << "daemon", QCoreApplication::translate("main", "Run in daemon mode.  For the main application to communicate with via dbus"));

  QCommandLineParser parser;
  parser.setApplicationDescription("DataJockey audio file importer");
  parser.addHelpOption();
  parser.addVersionOption();
  parser.addOption(daemonOption);
  parser.addPositionalArgument("files", QCoreApplication::translate("main", "The files to process."));

  parser.process(a);

  dj::Configuration * config = dj::Configuration::instance();
  config->load_default();

  QStringList files = parser.positionalArguments();
  FileProcessor * processor = new FileProcessor;

  if (!parser.isSet(daemonOption)) {
    DB * db = new DB(config->db_adapter(), config->db_name(), config->db_username(), config->db_password(), config->db_port(), config->db_host());

    QObject::connect(db, &DB::importError, [] (QString audioFilePath, QString errorMessage) {
      qDebug() << "error: " << errorMessage << " importing: " << audioFilePath << endl;
    });

    QSet<QString> filesToCheck;
    //actually locate which files are in the DB and which aren't
    foreach (QString file, files) {
      QFileInfo fileInfo(file);
      if (fileInfo.isDir()) {
        scanDir(QDir(file), filesToCheck);
      } else if (fileInfo.isFile()) {
        QString ext = fileInfo.suffix();
        if (dj::audio_file_extensions.contains(ext, Qt::CaseInsensitive)) {
          filesToCheck.insert(fileInfo.canonicalFilePath());
        } else {
          qDebug() << "isn't a supported audio file: " << file << endl;
        }
      } else {
        qDebug() << "isn't a file or directory: " << file << endl;
      }
    }

    QStringList filesToProcess;
    foreach(QString file, filesToCheck.toList()) {
      if (db->work_find_by_audio_file_location(file) == 0)
        filesToProcess.push_back(file);
    }

    processor->addFiles(filesToProcess);

    QObject::connect(processor, &FileProcessor::fileCreated, db, &DB::import);
    //QObject::connect(processor, &FileProcessor::complete, &a, QCoreApplication::quit);
    QTimer::singleShot(0, processor, SLOT(process()));
  }

  return a.exec();
}
