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
    djaudio::LoaderThread * loader = new djaudio::LoaderThread(p, this);
    mLoaders.push_back(loader);
    connect(loader,
        SIGNAL(playerValueChangedInt(int, QString, int)),
        SIGNAL(playerValueChangedInt(int,QString, int)));
    connect(loader,
        SIGNAL(playerValueChangedString(int, QString, QString)),
        SIGNAL(playerValueChangedString(int,QString, QString)));
    connect(loader,
      SIGNAL(loadComplete(int, djaudio::AudioBufferPtr, djaudio::BeatBufferPtr)),
      SIGNAL(playerBuffersChanged(int, djaudio::AudioBufferPtr, djaudio::BeatBufferPtr)));
  }

  try {
    QString audio_file_location;
    QString annotation_file_location;
    if (mDB->find_locations_by_id(mWorkID, audio_file_location, annotation_file_location)) {
      QString songinfo("$title\n$artist");
      mDB->format_string_by_id(mWorkID, songinfo);
      emit(playerValueChangedString(player, "loading_work", songinfo));
      emit(playerValueChangedInt(player, "loading_work", mWorkID));
      mLoaders[player]->load(audio_file_location, annotation_file_location, songinfo);
    }
  } catch (std::exception& e) {
    emit(playerLoadError(player, QString(e.what())));
  }
}

void AudioLoader::selectWork(int id) {
  mWorkID = id;
}

