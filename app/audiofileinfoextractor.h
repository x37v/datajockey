#ifndef AUDIOFILEINFOEXTRACTOR_H
#define AUDIOFILEINFOEXTRACTOR_H

#include <QObject>

class BeatExtractor;

class AudioFileInfoExtractor : public QObject {
  Q_OBJECT
  public:
    explicit AudioFileInfoExtractor(QObject *parent = 0);
    virtual ~AudioFileInfoExtractor();

  signals:
    void fileCreated(QString audioFilePath, QString annotationFilePath);
    void error(QString audioFilePath, QString errorMessage);

  public slots:
    void processAudioFile(QString audioFileName);
  private:
    BeatExtractor * mBeatExtractor;
};

#endif // AUDIOFILEINFOEXTRACTOR_H
