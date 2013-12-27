#include "loaderthread.h"
#include <QMutexLocker>

using namespace djaudio;

LoaderThread::LoaderThread(QObject * parent) :
  QThread(parent),
  mMutex(QMutex::Recursive)
{ 
}

void LoaderThread::progress_callback(int percent, void *objPtr) {
  //XXX lock mutex?
  LoaderThread * self = (LoaderThread *)objPtr;
  //only every 5 percent
  if (percent % 5 == 0)
    self->relay_load_progress(percent);
}

void LoaderThread::abort() {
  QMutexLocker lock(&mMutex);
  if (mAudioBuffer)
    mAudioBuffer->abort_load();
  mAborted = true;
}

void LoaderThread::relay_load_progress(int percent) {
  emit(loadProgress(percent));
}

void LoaderThread::load(QString audio_file_location, QString annotation_file_location) {
  QMutexLocker lock(&mMutex);
  mAborted = false;

  if (isRunning()) {
    abort();
    wait();
  }

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
      if (!mBeatBuffer->load(mAnnotationFileName)) {
        emit(loadError("problem loading annotation file: " + mAnnotationFileName));
        mBeatBuffer.reset();
      }
    }

    if (mAudioBuffer->load(LoaderThread::progress_callback, this))
      emit(loadComplete(mAudioBuffer, mBeatBuffer));
    else
      emit(loadError("problem loading audio file: " + mAudioFileName));
  } catch (std::exception& e) {
    emit(loadError("problem loading audio file: " + mAudioFileName + " " + QString::fromStdString(e.what())));
  }
}

