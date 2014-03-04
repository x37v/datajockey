#ifndef AUDIOFILEINFOEXTRACTOR_H
#define AUDIOFILEINFOEXTRACTOR_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariant>

class BeatExtractor;

class AudioFileInfoExtractor : public QObject {
  Q_OBJECT
  public:
    explicit AudioFileInfoExtractor(QObject *parent = 0);
    virtual ~AudioFileInfoExtractor();

  signals:
    void fileCreated(QString audioFilePath, QString annotationFilePath, QHash<QString, QVariant> tagData);
    void error(QString audioFilePath, QString errorMessage);

  public slots:
    void processAudioFile(QString audioFileName);
  private:
    BeatExtractor * mBeatExtractor;
};

#endif // AUDIOFILEINFOEXTRACTOR_H
