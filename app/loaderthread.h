#ifndef DATAJOCKEY_AUDIO_LOADER_THREAD_H
#define DATAJOCKEY_AUDIO_LOADER_THREAD_H

#include <QThread>
#include <QMutex>
#include <QString>
#include "audio/annotation.hpp"
#include "audio/audiobuffer.hpp"

namespace djaudio {
  class LoaderThread : public QThread {
    Q_OBJECT
    public:
      LoaderThread(int player_index, QObject * parent = nullptr);
      void load(QString audio_file_location, QString annotation_file_location, QString songinfo);
      void run();
      static void progress_callback(int percent, void *objPtr);
      void abort();
    signals:
      void playerValueChangedInt(int player, QString name, int value);
      void playerValueChangedString(int player, QString name, QString value);
      void loadComplete(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
    protected:
      void relay_load_progress(int percent);
    private:
      int mPlayerIndex = 0;

      AudioBufferPtr mAudioBuffer;
      BeatBufferPtr mBeatBuffer;

      QString mAudioFileName;
      QString mAnnotationFileName;
      QString mSongInfo;

      QMutex mMutex;
      bool mAborted;
  };
}

#endif
