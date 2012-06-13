#ifndef DATAJOCKEY_AUDIO_LOADER_THREAD_H
#define DATAJOCKEY_AUDIO_LOADER_THREAD_H

#include <QThread>
#include <QMutex>
#include <QString>
#include "beatbuffer.hpp"
#include "audiobuffer.hpp"

namespace dj {
   namespace audio {
      class LoaderThread : public QThread {
         Q_OBJECT
         public:
            LoaderThread();
            void load(int player_index, QString audio_file_location, QString annotation_file_location);
            void run();
            static void progress_callback(int percent, void *objPtr);
            void abort();
         signals:
            void load_progress(int player_index, int percent);
            void load_complete(int player_index,
                  AudioBufferPtr audio_buffer,
                  BeatBufferPtr beat_buffer);
            void load_error(int player_index, QString explanation);
         protected:
            void relay_load_progress(int percent);
         private:
            AudioBufferPtr mAudioBuffer;
            BeatBufferPtr mBeatBuffer;
            int mPlayerIndex;

            QString mAudioFileName;
            QString mAnnotationFileName;

            QMutex mMutex;
            bool mAborted;
      };
   }
}

#endif
