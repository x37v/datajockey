#ifndef DATAJOCKEY_AUDIO_LOADER_THREAD_H
#define DATAJOCKEY_AUDIO_LOADER_THREAD_H

#include <QThread>
#include <QMutex>
#include <QString>
#include "audiocontroller.hpp"

namespace DataJockey {
   namespace Audio {

      class AudioBuffer;
      class AudioController::AudioLoaderThread : public QThread {
         public:
            AudioLoaderThread(AudioController * controller);
            DataJockey::Audio::AudioBuffer * load(QString location);
            void run();
            static void progress_callback(int percent, void *objPtr);
            void abort();
            const QString& file_name();
         protected:
            DataJockey::Audio::AudioBuffer * mAudioBuffer;
            AudioController * controller();
         private:
            QString mFileName;
            AudioController * mAudioController;
            QMutex mMutex;
            bool mAborted;
      };
   }
}

#endif
