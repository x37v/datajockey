#include "fileprocessor.h"
#include "audiofileinfoextractor.h"

FileProcessor::FileProcessor(QObject *parent) :
  QObject(parent)
{
}

void FileProcessor::addFiles(QStringList files) {
  mFiles.append(files);
}

void FileProcessor::process() {
  AudioFileInfoExtractor * extractor = new AudioFileInfoExtractor(this);
  foreach (QString file, mFiles) {
    extractor->processAudioFile(file);
  }
  extractor->deleteLater();
  emit(complete());
}

