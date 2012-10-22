#ifndef DATAJOCKEY_IMPORTER_APP_HPP
#define DATAJOCKEY_IMPORTER_APP_HPP

#include <QCoreApplication>
#include <vector>
#include <string>
#include "importer.hpp"
#include <QString>
#include <QList>

class ImporterApplication : public QCoreApplication {
   Q_OBJECT
   public:
      ImporterApplication(int & argc, char ** argv);
      void import_paths(std::vector<std::string>& paths, QList<QRegExp> ignore_patterns = QList<QRegExp>());
   public slots:
      void cleanup();
   private:
      std::vector<std::string> mImportPaths;
      dj::util::Importer * mImporter;
};

#endif
