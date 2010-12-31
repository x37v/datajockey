#include "audioloaderthread.hpp"
#include <QMutexLocker>

using namespace DataJockey::Audio;

AudioController::AudioLoaderThread::AudioLoaderThread(AudioController * controller)
: mAudioController(controller), mFileName(), mAudioBuffer(NULL), mMutex(QMutex::Recursive) { }

void AudioController::AudioLoaderThread::progress_callback(int percent, void *objPtr) {
   AudioLoaderThread * self = (AudioLoaderThread *)objPtr;
   self->relay_load_progress(self->file_name(), percent);
}

void AudioController::AudioLoaderThread::abort() {
   QMutexLocker lock(&mMutex);
   if (mAudioBuffer)
      mAudioBuffer->abort_load();
   mAborted = true;
}

const QString& AudioController::AudioLoaderThread::file_name() {
   QMutexLocker lock(&mMutex);
   return mFileName;
}

void AudioController::AudioLoaderThread::relay_load_progress(QString fileName, int percent) {
   emit(load_progress(fileName, percent));
}

DataJockey::Audio::AudioBuffer * AudioController::AudioLoaderThread::load(QString location){
   QMutexLocker lock(&mMutex);
   mAborted = false;

   if (isRunning()) {
      abort();
      wait();
      //TODO what if it is loading?  how do we deal with a hanging reference?
   }
   mFileName = location;

   try {
      mAudioBuffer = new DataJockey::Audio::AudioBuffer(location.toStdString());
      start();
   } catch (...){ return NULL; }
   return mAudioBuffer;
}

void AudioController::AudioLoaderThread::run() {
   if (mAudioBuffer) {
      mAudioBuffer->load(AudioLoaderThread::progress_callback, this);
      {
         QMutexLocker lock(&mMutex);
         if (!mAborted) {
            //tell the controller that the load is complete.
            //if it doesn't use the buffer then delete it
            if (!controller()->audio_file_load_complete(mFileName, mAudioBuffer))
               delete mAudioBuffer;
         } else
            delete mAudioBuffer;
         //cleanup
         mAudioBuffer = NULL;
         mFileName = QString();
      }
   }
}

AudioController * AudioController::AudioLoaderThread::controller() {
   return mAudioController;
}

