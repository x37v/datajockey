#ifndef DJAUDIOLOADER_H
#define DJAUDIOLOADER_H

#include <QObject>
#include <QList>
#include "audio/audiobuffer.hpp"
#include "loaderthread.h"
#include "db.h"

class AudioLoader : public QObject {
  Q_OBJECT
  public:
    explicit AudioLoader(DB * db, QObject *parent = 0);
  public slots:
    void playerTrigger(int player, QString name);
    void selectWork(int id);
  signals:
    void playerBuffersChanged(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
    void playerValueChangedInt(int player, QString name, int value);
    void playerValueChangedString(int player, QString name, QString value);

    void playerLoadError(int player, QString errormsg);
  private:
    QList<djaudio::LoaderThread *> mLoaders;
    DB * mDB;
    int mWorkID = 0;
};

#endif
