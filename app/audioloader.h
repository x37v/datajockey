#ifndef DJAUDIOLOADER_H
#define DJAUDIOLOADER_H

#include <QObject>
#include <QList>
#include "audio/audiobuffer.hpp"
#include "audio/beatbuffer.hpp"
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
    void playerLoadingInfo(int player, QString songinfo);
    void playerLoaded(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
    void playerLoadedInfo(int player, QString songinfo);

    void playerLoadError(int player, QString errormsg);
    void playerLoadProgress(int player, int percent);
  private:
    QList<djaudio::LoaderThread *> mLoaders;
    DB * mDB;
    int mWorkID = 0;
};

#endif
