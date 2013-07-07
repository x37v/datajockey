#ifndef DJ_IMPORTER_HPP
#define DJ_IMPORTER_HPP

#include <QObject>
#include <QStringList>
#include <QHash>
#include <QVariant>
#include <QRegExp>
#include <QThreadPool>
#include <QThread>
#include <QMutex>

#include "beatbuffer.hpp"
#include "db.hpp"

namespace dj {
  namespace util {
    using dj::audio::BeatBufferPtr;

    class AudioInfoExtractor : public QObject {
      Q_OBJECT
      public:
        AudioInfoExtractor(QObject * parent = NULL);
      public slots:
        void extract_data(const QString& audio_file);
      signals:
        void extracted_data(QString audio_file, QHash<QString, QVariant> tag_data, BeatBufferPtr beat_data);
        void extraction_failed(QString audio_file, QString message);
    };

    class NewSongFinder : public QThread {
      Q_OBJECT
      public:
        NewSongFinder(QObject * parent = NULL);
        virtual void run();
      public slots:
        void watch_dir(const QString& dir);
        void ignore_pattern(QRegExp ignore_pattern);
      signals:
        void found_files(QStringList files);
      private:
        QList<QRegExp> mIgnorePatterns;
        QStringList mWatchDirs;
        QMutex mMutex;
    };

    class Importer : public QObject {
      Q_OBJECT
      public:
        Importer(dj::model::DB *db, QObject * parent = NULL);
      public slots:
        void import(const QStringList& file_list, bool recurse_directories = true, QList<QRegExp> ignore_patterns = QList<QRegExp>());

        void import_data(QString audio_file, QHash<QString, QVariant> tag_data, BeatBufferPtr beat_data);
        void report_failure(QString audio_file, QString message);
      signals:
        void finished();
        void progress_changed(int progress);
        void imported_file(QString path, int work_id);
        void import_failed(QString path, QString message);
      private:
        dj::model::DB * mDB;
        QThreadPool mExtractorPool;
        int mImportingCount;
        void decrement_count();
    };
  }
}

#endif
