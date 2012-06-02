#include "audioloaderthread.hpp"
#include <QMutexLocker>

using namespace dj::audio;

AudioModel::AudioLoaderThread::AudioLoaderThread(AudioModel * model)
: mAudioBuffer(NULL), mAudioModel(model), mFileName(), mMutex(QMutex::Recursive) { }

void AudioModel::AudioLoaderThread::progress_callback(int percent, void *objPtr) {
   AudioLoaderThread * self = (AudioLoaderThread *)objPtr;
   self->relay_load_progress(self->file_name(), percent);
}

void AudioModel::AudioLoaderThread::abort() {
   QMutexLocker lock(&mMutex);
   if (mAudioBuffer)
      mAudioBuffer->abort_load();
   mAborted = true;
}

const QString& AudioModel::AudioLoaderThread::file_name() {
   QMutexLocker lock(&mMutex);
   return mFileName;
}

void AudioModel::AudioLoaderThread::relay_load_progress(QString fileName, int percent) {
   emit(load_progress(fileName, percent));
}

dj::audio::AudioBuffer * AudioModel::AudioLoaderThread::load(QString location){
   QMutexLocker lock(&mMutex);
   mAborted = false;

   if (isRunning()) {
      abort();
      wait();
      //TODO what if it is loading?  how do we deal with a hanging reference?
   }
   mFileName = location;

   try {
      mAudioBuffer = new dj::audio::AudioBuffer(location.toStdString());
      start();
   } catch (...){ return NULL; }
   return mAudioBuffer;
}

//TODO report errors
void AudioModel::AudioLoaderThread::run() {
   if (mAudioBuffer) {
      mAudioBuffer->load(AudioLoaderThread::progress_callback, this);
      if(mAudioBuffer->valid()) {
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
      } else {
         QMutexLocker lock(&mMutex);
         delete mAudioBuffer;
         //cleanup
         mAudioBuffer = NULL;
         mFileName = QString();
      }
   }
}

AudioModel * AudioModel::AudioLoaderThread::model() {
   return mAudioModel;
}

