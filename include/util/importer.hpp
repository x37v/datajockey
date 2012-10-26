#ifndef DJ_IMPORTER_HPP
#define DJ_IMPORTER_HPP

#include <QObject>
#include <QStringList>
#include <QtCore>
#include <QRegExp>

//XXX why doesn't QtConcurrent::QFutureWatcher<void> work?
using namespace QtConcurrent;

namespace dj {
  namespace util {
    class Importer : public QObject {
      Q_OBJECT
      public:
        Importer(QObject * parent = NULL);
      public slots:
        void import(const QStringList& file_list, bool recurse_directories = true, QList<QRegExp> ignore_patterns = QList<QRegExp>());
        void import_blocking(const QString& audio_file);
      signals:
        void finished();
        void progress_changed(int progress);
      private:
        QFutureWatcher<void> * mFutureWatcher;
        QStringList mFileList;
    };
  }
}

#endif
