#include "fileprocessor.h"
#include "audiofileinfoextractor.h"
#include <QDir>
#include <QtDebug>
#include <QRunnable>

class ProcessTask : public QRunnable {
  public:
    ProcessTask(QString filePath, FileProcessor * parent) : mFile(filePath) {
      mExtractor = new AudioFileInfoExtractor(nullptr);
      QObject::connect(mExtractor, &AudioFileInfoExtractor::fileCreated, parent, &FileProcessor::fileCreated);
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

  for(int i = 0; i < mFiles.size(); i++) {
    QString file = mFiles[i];
    QDir dir(file);
    if (dir.exists()) {
      QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
      foreach (QString e, entries)
        mFiles.append(dir.absoluteFilePath(e));
      continue;
    } else if (QFile::exists(file)) {
      ProcessTask * task = new ProcessTask(file, this);
      mThreadPool->start(task);
    } else {
      qDebug() << "doesn't exist " << file << endl;
    }
  }
  mThreadPool->waitForDone();
  emit(complete());
}

