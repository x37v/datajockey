#include "audioloaderthread.hpp"
#include "audiomodel.hpp"
#include "audiobuffer.hpp"
#include <QMetaObject>
#include <QMutexLocker>

using namespace DataJockey::Internal;

AudioLoaderThread::AudioLoaderThread(AudioModel * model)
: mAudioModel(model), mFileName(), mAudioBuffer(NULL), mMutex(QMutex::Recursive) { }

void AudioLoaderThread::progress_callback(int percent, void *objPtr) {
   AudioLoaderThread * self = (AudioLoaderThread *)objPtr;
   QMetaObject::invokeMethod(self->model(),
         "relay_player_audio_file_load_progress",
         Qt::QueuedConnection, 
         Q_ARG(QString, self->file_name()),
         Q_ARG(int, percent));
}

void AudioLoaderThread::abort() {
   QMutexLocker lock(&mMutex);
   if (mAudioBuffer)
      mAudioBuffer->abort_load();
   mAborted = true;
}

const QString& AudioLoaderThread::file_name() {
   QMutexLocker lock(&mMutex);
   return mFileName;
}

DataJockey::AudioBuffer * AudioLoaderThread::load(QString location){
   QMutexLocker lock(&mMutex);
   mAborted = false;

   if (isRunning()) {
      abort();
      wait();
      //TODO what if it is loading?  how do we deal with a hanging reference?
   }
   mFileName = location;

   try {
      mAudioBuffer = new DataJockey::AudioBuffer(location.toStdString());
      start();
   } catch (...){ return NULL; }
   return mAudioBuffer;
}

void AudioLoaderThread::run() {
   if (mAudioBuffer) {
      mAudioBuffer->load(AudioLoaderThread::progress_callback, this);
      {
         QMutexLocker lock(&mMutex);
         if (!mAborted) {
            //tell the model that the load is complete.
            //if it doesn't use the buffer then delete it
            if (!model()->audio_file_load_complete(mFileName, mAudioBuffer))
               delete mAudioBuffer;
         } else
            delete mAudioBuffer;
         //cleanup
         mAudioBuffer = NULL;
         mFileName = QString();
      }
   }
}

DataJockey::AudioModel * AudioLoaderThread::model() {
   return mAudioModel;
}

