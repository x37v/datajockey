#include "fileprocessor.h"
#include "audiofileinfoextractor.h"
#include <QDir>
#include <QtDebug>

FileProcessor::FileProcessor(QObject *parent) :
  QObject(parent)
{
}

void FileProcessor::addFiles(QStringList files) {
  mFiles.append(files);
}

void FileProcessor::process() {
  AudioFileInfoExtractor * extractor = new AudioFileInfoExtractor(this);
  connect(extractor, &AudioFileInfoExtractor::fileCreated, this, &FileProcessor::fileCreated);

  for(int i = 0; i < mFiles.size(); i++) {
    QString file = mFiles[i];
    QDir dir(file);
    if (dir.exists()) {
      QStringList entries = dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
      foreach (QString e, entries)
        mFiles.append(dir.absoluteFilePath(e));
      continue;
    } else if (QFile::exists(file)) {
      extractor->processAudioFile(file);
    } else {
      qDebug() << "doesn't exist " << file << endl;
    }
  }
  extractor->deleteLater();
  emit(complete());
}

