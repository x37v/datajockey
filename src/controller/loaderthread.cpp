#include "loaderthread.hpp"
#include <QMutexLocker>

using namespace dj::audio;

LoaderThread::LoaderThread()
: mPlayerIndex(0), mMutex(QMutex::Recursive) { 
}

void LoaderThread::progress_callback(int percent, void *objPtr) {
   //XXX lock mutex?
   LoaderThread * self = (LoaderThread *)objPtr;
   self->relay_load_progress(percent);
}

void LoaderThread::abort() {
   QMutexLocker lock(&mMutex);
   if (mAudioBuffer)
      mAudioBuffer->abort_load();
   mAborted = true;
}

void LoaderThread::relay_load_progress(int percent) {
   emit(load_progress(mPlayerIndex, percent));
}

void LoaderThread::load(int player_index, QString audio_file_location, QString annotation_file_location) {
   QMutexLocker lock(&mMutex);
   mAborted = false;

   if (isRunning()) {
      abort();
      wait();
   }

   mPlayerIndex = player_index;
   mAudioFileName = audio_file_location;
   mAnnotationFileName = annotation_file_location;
   start();
}

void LoaderThread::run() {
   setPriority(QThread::NormalPriority);
   try {
      mAudioBuffer.reset();
      mBeatBuffer.reset();

      mBeatBuffer = BeatBufferPtr(new BeatBuffer);
      mAudioBuffer = AudioBufferPtr(new AudioBuffer(mAudioFileName.toStdString()));

      if (!mAnnotationFileName.isEmpty()) {
         if (!mBeatBuffer->load(mAnnotationFileName.toStdString())) {
            emit(load_error(mPlayerIndex, "problem loading annotation file: " + mAnnotationFileName));
            mBeatBuffer.reset();
         }
      }

      if (mAudioBuffer->load(LoaderThread::progress_callback, this))
         emit(load_complete(mPlayerIndex, mAudioBuffer, mBeatBuffer));
      else
         emit(load_error(mPlayerIndex, "problem loading audio file: " + mAudioFileName));
   } catch (std::exception& e) {
      emit(load_error(mPlayerIndex, "problem loading audio file: " + mAudioFileName + " " + QString::fromStdString(e.what())));
   }
}

