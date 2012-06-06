#include "importer.hpp"
#include "beatbuffer.hpp"
#include "beatextractor.hpp"
#include "audiofiletag.hpp"
#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "db.hpp"
#include "defines.hpp"
#include <QFileInfo>
#include <QFutureWatcher>
#include <stdexcept>

using namespace dj;
using namespace dj::util;
using namespace QtConcurrent;

namespace {
   const QStringList valid_file_types = (QStringList() << "flac" << "mp3" << "ogg");

   QStringList recurse_dirs(const QStringList& file_list) {
      QStringList files;

      foreach(const QString &item, file_list) {
         QFileInfo info(item);
         if (!info.exists())
            continue;
         if (info.isDir()) {
            QDir dir(item);
            QStringList entries = dir.entryList(QDir::AllEntries | QDir::Readable | QDir::NoDotAndDotDot);
            for(QStringList::iterator it = entries.begin(); it != entries.end(); it++)
               *it = dir.filePath(*it);
            files << recurse_dirs(entries);
         } else
            files << item;
      }
      return files;
   }

#include <iostream>
   using namespace std;

   void import_file(const QString& audio_file_path) {
      try {
         if (model::db::work::find_by_audio_file_location(audio_file_path) != 0) {
            qDebug() << audio_file_path << " already in database, skipping";
            return;
         }
         //grab tag data
         QHash<QString, QVariant> tag_data;
         audiofile_tag::extract(audio_file_path, tag_data);

         //grab audio
         audio::AudioBuffer audio_buffer(audio_file_path.toStdString());
         if(!audio_buffer.load())
            throw(std::runtime_error(DJ_FILEANDLINE + " failed to load audio buffer " + audio_file_path.toStdString()));
         tag_data["milliseconds"] = static_cast<unsigned int>(static_cast<double>(audio_buffer.length() * 1000) / static_cast<double>(audio_buffer.sample_rate()));

         //extract beats
         audio::BeatBuffer beat_buffer;
         BeatExtractor extractor;
         extractor.process(audio_buffer, beat_buffer);

         //create annotation file
         audio::Annotation annotation;
         annotation.update_attributes(tag_data);
         annotation.beat_buffer(beat_buffer);

         //create db entry
         int work_id = model::db::work::create(
               tag_data,
               audio_file_path);

         //write annotation file and store the location in the db
         QString annotation_file_location = annotation.default_file_location(work_id);
         annotation.write_file(annotation_file_location);
         model::db::work::update_attribute(
               work_id,
               "annotation_file_location",
               annotation_file_location);

      } catch (std::exception& e) {
         qDebug() << "failed to import file " << audio_file_path << " " << e.what();
      } catch (...) {
         qDebug() << "failed to import file " << audio_file_path;
      }
   }
}

Importer::Importer(QObject * parent) : QObject(parent) {
   mFutureWatcher = new QFutureWatcher<void>;
   QObject::connect(
         mFutureWatcher,
         SIGNAL(finished()),
         SIGNAL(finished()));
}

void Importer::import(const QStringList& file_list, bool recurse_directories) {
   QStringList files = (recurse_directories ? recurse_dirs(file_list) : file_list);

   if (mFutureWatcher->isRunning()) {
      //XXX should we cancel?
      mFutureWatcher->cancel();
      mFutureWatcher->waitForFinished();
   }

   mFileList.clear();
   foreach(const QString &item, files) {
      QFileInfo info(item);
      if (!info.exists() || !info.isFile() || !valid_file_types.contains(info.suffix().toLower()))
         continue;
      mFileList << item;
   }
   mFutureWatcher->setFuture(QtConcurrent::map(mFileList, import_file));
}

void Importer::import_blocking(const QString& audio_file) {
   import_file(audio_file);
}

