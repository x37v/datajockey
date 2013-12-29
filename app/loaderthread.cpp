#include "loaderthread.h"
#include <QMutexLocker>

using namespace djaudio;

#include <iostream>
using namespace std;

LoaderThread::LoaderThread(int player_index, QObject * parent) :
  QThread(parent),
  mPlayerIndex(player_index),
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
  emit(playerValueChangedInt(mPlayerIndex, "load_percent", percent));
}

void LoaderThread::load(QString audio_file_location, QString annotation_file_location, QString songinfo) {
  QMutexLocker lock(&mMutex);
  mAborted = false;
  mSongInfo = songinfo;

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
        emit(playerValueChangedString(mPlayerIndex, "load_error", "problem loading annotation file: " + mAnnotationFileName));
        mBeatBuffer.reset();
      }
    }

    if (mAudioBuffer->load(LoaderThread::progress_callback, this)) {
      emit(loadComplete(mPlayerIndex, mAudioBuffer, mBeatBuffer));
      emit(playerValueChangedString(mPlayerIndex, "work_info", mSongInfo));
    } else
      emit(playerValueChangedString(mPlayerIndex, "load_error", "problem loading audio file: " + mAudioFileName));
  } catch (std::exception& e) {
    emit(playerValueChangedString(mPlayerIndex, "load_error", "problem loading audio file: " + mAudioFileName + " " + QString::fromStdString(e.what())));
  }
}

