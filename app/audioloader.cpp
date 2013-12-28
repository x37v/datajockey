#include "audioloader.h"

using namespace djaudio;

AudioLoader::AudioLoader(DB * db, QObject * parent) : 
  QObject(parent),
  mDB(db)
{
  qRegisterMetaType<djaudio::AudioBufferPtr>("djaudio::AudioBufferPtr");
  qRegisterMetaType<djaudio::BeatBufferPtr>("djaudio::BeatBufferPtr");
}

void AudioLoader::playerTrigger(int player, QString name) {
  if (player < 0 || name != "load")
    return;

  //build up loaders
  for (int p = mLoaders.size(); p <= player; p++) {
    djaudio::LoaderThread * loader = new djaudio::LoaderThread(this);
    mLoaders.push_back(loader);
    connect(loader,
      &djaudio::LoaderThread::loadProgress, 
      [p, this](int percent) {
        emit(playerLoadProgress(p, percent));
      });
    connect(loader,
      &djaudio::LoaderThread::loadComplete, 
      [p, this](AudioBufferPtr audio_buffer, BeatBufferPtr beat_buffer, QString songinfo) {
        emit(playerLoaded(p, audio_buffer, beat_buffer));
        emit(playerLoadedInfo(p, songinfo));
      });
    connect(loader,
      &djaudio::LoaderThread::loadError, 
      [p, this](QString error) {
        emit(playerLoadError(p, error));
      });
  }

  try {
    QString audio_file_location;
    QString annotation_file_location;
    if (mDB->find_locations_by_id(mWorkID, audio_file_location, annotation_file_location)) {
      QString songinfo("$title\n$artist");
      mDB->format_string_by_id(mWorkID, songinfo);
      emit(playerLoadingInfo(player, songinfo));
      mLoaders[player]->load(audio_file_location, annotation_file_location, songinfo);
    }
  } catch (std::exception& e) {
    emit(playerLoadError(player, QString(e.what())));
  }
}

void AudioLoader::selectWork(int id) {
  mWorkID = id;
}

