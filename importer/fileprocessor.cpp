#include "fileprocessor.h"
#include "audiofileinfoextractor.h"
#include <QDir>
#include <QtDebug>
#include <QRunnable>
#include <QTimer>

class ProcessTask : public QRunnable {
  public:
    ProcessTask(QString filePath, FileProcessor * parent) : mFile(filePath) {
      mExtractor = new AudioFileInfoExtractor(nullptr);
      QObject::connect(mExtractor, &AudioFileInfoExtractor::fileCreated, parent, &FileProcessor::reportFileCreated);
      QObject::connect(mExtractor, &AudioFileInfoExtractor::error, parent, &FileProcessor::reportFileFailed);
    }

    virtual ~ProcessTask() {
      delete mExtractor;
    }

    virtual void run() {
      mExtractor->processAudioFile(mFile);
    }
  private:
    QString mFile;
    AudioFileInfoExtractor * mExtractor;
};


FileProcessor::FileProcessor(QObject *parent) :
  QObject(parent)
{
  mThreadPool = new QThreadPool(this);
}

void FileProcessor::addFiles(QStringList files) {
  mFiles.append(files);
}

void FileProcessor::process() {
  QStringList files = mFiles;
  mFiles.clear();
  for(int i = 0; i < files.size(); i++) {
    QString file = files[i];
    QDir dir(file);
    if (dir.exists()) {
      qDebug() << "is a directory " << file << endl;
    } else if (QFile::exists(file)) {
      ProcessTask * task = new ProcessTask(file, this);
      mThreadPool->start(task);
    } else {
      qDebug() << "doesn't exist " << file << endl;
    }
  }

  //poll for done state
  QTimer * done_timer = new QTimer(this);
  done_timer->setInterval(100);
  connect(done_timer, &QTimer::timeout, [this, done_timer] () {
    if (mThreadPool->waitForDone(1)) {
      emit(complete());
      done_timer->stop();
    }
  });
  done_timer->start(10);
}

void FileProcessor::reportFileCreated(QString audioFilePath, QString annotationFilePath, QHash<QString, QVariant> tagData) {
  emit(fileCreated(audioFilePath, annotationFilePath, tagData));
}

void FileProcessor::reportFileFailed(QString audioFilePath, QString message) {
  emit(fileFailed(audioFilePath, message));
}

