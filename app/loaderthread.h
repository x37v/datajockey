#ifndef DATAJOCKEY_AUDIO_LOADER_THREAD_H
#define DATAJOCKEY_AUDIO_LOADER_THREAD_H

#include <QThread>
#include <QMutex>
#include <QString>
#include "audio/beatbuffer.hpp"
#include "audio/audiobuffer.hpp"

namespace djaudio {
  class LoaderThread : public QThread {
    Q_OBJECT
    public:
      LoaderThread(QObject * parent = nullptr);
      void load(QString audio_file_location, QString annotation_file_location);
      void run();
      static void progress_callback(int percent, void *objPtr);
      void abort();
    signals:
      void loadProgress(int percent);
      void loadComplete(AudioBufferPtr audio_buffer, BeatBufferPtr beat_buffer);
      void loadError(QString explanation);
    protected:
      void relay_load_progress(int percent);
    private:
      AudioBufferPtr mAudioBuffer;
      BeatBufferPtr mBeatBuffer;

      QString mAudioFileName;
      QString mAnnotationFileName;

      QMutex mMutex;
      bool mAborted;
  };
}

#endif
