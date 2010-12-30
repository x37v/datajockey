#ifndef DATAJOCKEY_AUDIO_LOADER_THREAD_H
#define DATAJOCKEY_AUDIO_LOADER_THREAD_H

#include <QThread>
#include <QMutex>
#include <QString>

namespace DataJockey {
   //forward declarations
   class AudioModel;
   class AudioBuffer;

   namespace Internal {
      class AudioLoaderThread : public QThread {
         public:
            AudioLoaderThread(AudioModel * model);
            DataJockey::AudioBuffer * load(QString location);
            void run();
            static void progress_callback(int percent, void *objPtr);
            void abort();
            const QString& file_name();
         protected:
            DataJockey::AudioBuffer * mAudioBuffer;
            AudioModel * model();
         private:
            QString mFileName;
            AudioModel * mAudioModel;
            QMutex mMutex;
            bool mAborted;
      };
   }
}

#endif
