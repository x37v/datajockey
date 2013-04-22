#include "importer.hpp"
#include "beatbuffer.hpp"
#include "beatextractor.hpp"
#include "audiofiletag.hpp"
#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "db.hpp"
#include "defines.hpp"
#include <QDir>
#include <QFileInfo>
#include <QRunnable>
#include <QDebug>
#include <stdexcept>
#include <iostream>

using namespace dj;
using namespace dj::util;

using std::cout;
using std::endl;

namespace {
  const QStringList valid_file_types = (QStringList() << "flac" << "mp3" << "ogg");

  QStringList recurse_dirs(const QStringList& file_list, QList<QRegExp> ignore_patterns) {
    QStringList files;

    foreach(const QString &item, file_list) {
      QFileInfo info(item);
      if (!info.exists()) {
        qWarning() << item << " not a valid file or directory, skipping" << endl;
        continue;
      }
      if (info.isDir()) {
        QDir dir(item);
        QStringList entries = dir.entryList(QDir::AllEntries | QDir::Readable | QDir::NoDotAndDotDot);
        for(QStringList::iterator it = entries.begin(); it != entries.end(); it++)
          *it = dir.filePath(*it);
        files << recurse_dirs(entries, ignore_patterns);
      } else {
        bool ignore = false;
        QRegExp regex;
        foreach (regex, ignore_patterns) {
          if (item.contains(regex)) {
            ignore = true;
            break;
          }
        }
        if (!ignore)
          files << info.canonicalFilePath();
      }
    }
    return files;
  }

  void import_file(const QString& audio_file_path) {
    unsigned int beat_smooth_iterations = 20; //XXX grab from config
    try {

      if (model::db::work::find_by_audio_file_location(audio_file_path) != 0) {
        //qWarning() << audio_file_path << " already in database, skipping";
        return;
      }

      //XXX not thread safe, might not print all at once, use signals?
      cout << "processing: " << audio_file_path.toStdString() << endl;

      //grab tag data
      QHash<QString, QVariant> tag_data;
      audiofile_tag::extract(audio_file_path, tag_data);

      //grab audio
      audio::AudioBufferPtr audio_buffer(new audio::AudioBuffer(audio_file_path.toStdString()));
      if(!audio_buffer->load())
        throw(std::runtime_error(DJ_FILEANDLINE + " failed to load audio buffer " + audio_file_path.toStdString()));
      tag_data["seconds"] = static_cast<unsigned int>(static_cast<double>(audio_buffer->length()) / static_cast<double>(audio_buffer->sample_rate()));

      //extract beats
      audio::BeatBufferPtr beat_buffer(new audio::BeatBuffer);
      BeatExtractor extractor;
      extractor.process(audio_buffer, beat_buffer);

      //smooth beat buffer
      if (beat_smooth_iterations)
        beat_buffer->smooth(beat_smooth_iterations);

      //create annotation file
      audio::Annotation annotation;
      annotation.update_attributes(tag_data);
      annotation.beat_buffer(beat_buffer);

      //create db entry
      int work_id = model::db::work::create(
          tag_data,
          audio_file_path);

      //create tempo descriptors
      if (beat_buffer->length() > 2) {
        try {
          double median, mean;
          beat_buffer->median_and_mean(median, mean);
          //XXX assuming 4/4 time
          if (median > 0.0) {
            model::db::work::descriptor_create_or_update(
                work_id,
                "tempo median",
                60.0 / median);
          }
          if (mean > 0.0) {
            model::db::work::descriptor_create_or_update(
                work_id,
                "tempo average",
                60.0 / mean);
          }
        } catch (std::exception& e) {
          qWarning() << "failed to create tempo descriptors for " << audio_file_path << " " << e.what();
        }
      }

      //create genre tag
      QVariant genre = tag_data["genre"];
      if (genre.isValid()) {
        try {
          model::db::work::tag(work_id, QString("genre"), genre.toString());
        } catch (std::exception& e) {
          qWarning() << "failed to genre tag for " << audio_file_path << " " << e.what();
        }
      }

      //write annotation file and store the location in the db
      QString annotation_file_location = annotation.default_file_location(work_id);
      annotation.write_file(annotation_file_location);
      model::db::work::update_attribute(
          work_id,
          "annotation_file_location",
          annotation_file_location);

      //XXX not thread safe, might not print all at once, use signals?
      cout << "complete: " << audio_file_path.toStdString() << endl;;

    } catch (std::exception& e) {
      qWarning() << "failed to import file " << audio_file_path << " " << e.what();
    } catch (...) {
      qWarning() << "failed to import file " << audio_file_path;
    }
  }

  void import_files(const QStringList& file_list) {
    foreach(const QString &file, file_list) {
      import_file(file);
    }
  }

  class ExtractorRunnable : public QRunnable {
    public:
      ExtractorRunnable(Importer * importer, QString audio_file_path) :
        mImporter(importer),
        mAudioFilePath(audio_file_path) { }
      virtual void run() {
        AudioInfoExtractor * extractor = new AudioInfoExtractor();

        QObject::connect(extractor, 
            SIGNAL(extracted_data(QString, QHash<QString, QVariant>, BeatBufferPtr)),
            mImporter,
            SLOT(import_data(QString, QHash<QString, QVariant>, BeatBufferPtr)));
        QObject::connect(extractor, 
            SIGNAL(extraction_failed(QString, QString)),
            mImporter,
            SLOT(report_failure(QString, QString)));

        extractor->extract_data(mAudioFilePath);
        extractor->deleteLater();
      }
    private:
      Importer * mImporter;
      QString mAudioFilePath;
  };
}

AudioInfoExtractor::AudioInfoExtractor(QObject * parent) : QObject(parent) {
}

void AudioInfoExtractor::extract_data(const QString& audio_file_path) {
  unsigned int beat_smooth_iterations = 20; //XXX grab from config
  try {
    //grab tag data
    QHash<QString, QVariant> tag_data;
    audiofile_tag::extract(audio_file_path, tag_data);

    //grab audio
    audio::AudioBufferPtr audio_buffer(new audio::AudioBuffer(audio_file_path.toStdString()));
    if (audio_buffer->channels() != 2) {
      emit(extraction_failed(audio_file_path, QString("%1 channel file, only stereo is supported").arg(audio_buffer->channels())));
      return;
    }

    if(!audio_buffer->load()) {
      emit(extraction_failed(audio_file_path, "failed to load audio buffer"));
      return;
    }

    tag_data["seconds"] = static_cast<unsigned int>(static_cast<double>(audio_buffer->length()) / static_cast<double>(audio_buffer->sample_rate()));

    //extract beats
    audio::BeatBufferPtr beat_buffer(new audio::BeatBuffer);
    BeatExtractor extractor;
    extractor.process(audio_buffer, beat_buffer);

    //smooth beat buffer
    if (beat_smooth_iterations)
      beat_buffer->smooth(beat_smooth_iterations);

    emit(extracted_data(audio_file_path, tag_data, beat_buffer));
  } catch (std::exception& e) {
    qWarning() << "failed to extract data from file " << audio_file_path << " " << e.what();
    emit(extraction_failed(audio_file_path, QString::fromStdString(e.what())));
  } catch (...) {
    qWarning() << "failed to extract data from file " << audio_file_path;
    emit(extraction_failed(audio_file_path, "unknown error"));
  }
}

Importer::Importer(QObject * parent) : QObject(parent), mImportingCount(0) {
}

void Importer::import(const QStringList& file_list, bool recurse_directories, QList<QRegExp> ignore_patterns) {
  QStringList files = (recurse_directories ? recurse_dirs(file_list, ignore_patterns) : file_list);
  foreach(const QString &item, files) {
    QFileInfo info(item);
    if (!info.exists() || !info.isFile() || !valid_file_types.contains(info.suffix().toLower()))
      continue;
    if (model::db::work::find_by_audio_file_location(item) != 0)
      continue;

    //push this job
    ExtractorRunnable * task = new ExtractorRunnable(this, item);
    task->setAutoDelete(true);
    mImportingCount++;

    cout << "queuing: " << qPrintable(item) << endl;
    mExtractorPool.start(task);
  }

  //we might not have queued anything
  if (mImportingCount == 0)
    emit(finished());
}

void Importer::import_data(QString audio_file_path, QHash<QString, QVariant> tag_data, BeatBufferPtr beat_buffer) {
  try {
    if (model::db::work::find_by_audio_file_location(audio_file_path) != 0) {
      qWarning() << audio_file_path << " already in database, skipping";
      decrement_count();
      return;
    }

    //create annotation file data
    audio::Annotation annotation;
    annotation.update_attributes(tag_data);
    annotation.beat_buffer(beat_buffer);

    //create db entry
    int work_id = model::db::work::create(
        tag_data,
        audio_file_path);

    //create tempo descriptors
    if (beat_buffer->length() > 2) {
      try {
        double median, mean;
        beat_buffer->median_and_mean(median, mean);
        //XXX assuming 4/4 time
        if (median > 0.0) {
          model::db::work::descriptor_create_or_update(
              work_id,
              "tempo median",
              60.0 / median);
        }
        if (mean > 0.0) {
          model::db::work::descriptor_create_or_update(
              work_id,
              "tempo average",
              60.0 / mean);
        }
      } catch (std::exception& e) {
        qWarning() << "failed to create tempo descriptors for " << audio_file_path << " " << e.what();
      }
    }

    //create genre tag
    QVariant genre = tag_data["genre"];
    if (genre.isValid()) {
      try {
        model::db::work::tag(work_id, QString("genre"), genre.toString());
      } catch (std::exception& e) {
        qWarning() << "failed to create genre tag for " << audio_file_path << " " << e.what();
      }
    }

    //write annotation file and store the location in the db
    QString annotation_file_location = annotation.default_file_location(work_id);
    annotation.write_file(annotation_file_location);
    model::db::work::update_attribute(
        work_id,
        "annotation_file_location",
        annotation_file_location);

    emit(imported_file(audio_file_path, work_id));
  } catch (std::exception& e) {
    qWarning() << "failed to import file " << audio_file_path << " " << e.what();
    emit(import_failed(audio_file_path, QString::fromStdString(e.what())));
  } catch (...) {
    qWarning() << "failed to import file " << audio_file_path;
    emit(import_failed(audio_file_path, "unknown error"));
  }
  decrement_count();
}

void Importer::report_failure(QString audio_file, QString message) {
  emit(import_failed(audio_file, message));
  decrement_count();
}

void Importer::decrement_count() {
  mImportingCount--;
  if (mImportingCount == 0)
    emit(finished());
}

