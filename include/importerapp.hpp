#ifndef DATAJOCKEY_IMPORTER_APP_HPP
#define DATAJOCKEY_IMPORTER_APP_HPP

#include <QCoreApplication>
#include <string>
#include "importer.hpp"
#include <QString>
#include <QList>
#include <QStringList>

class ImporterApplication : public QCoreApplication {
  Q_OBJECT
  public:
    ImporterApplication(int & argc, char ** argv);
    void import_paths(QStringList paths, QList<QRegExp> ignore_patterns = QList<QRegExp>());
  public slots:
    void cleanup();
    void report_success(QString path, int work_id);
    void report_failure(QString path, QString message);
  protected slots:
    void import();
  private:
    QStringList mImportPaths;
    QList<QRegExp> mIgnorePatterns;
    dj::util::Importer * mImporter;
};

#endif
