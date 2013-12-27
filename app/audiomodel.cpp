#include "audiomodel.h"
#include "defines.hpp"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

AudioModel::AudioModel(QObject *parent) :
  QObject(parent)
{
  mAudioIO = djaudio::AudioIO::instance();
  mMaster  = djaudio::Master::instance();

  for(int i = 0; i < mNumPlayers; i++) {
    mMaster->add_player();
  }
}

AudioModel::~AudioModel() {
  run(false);
}

void AudioModel::playerSetValueDouble(int player, QString name, double v) {
  if (!inRange(player))
    return;
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueInt(int player, QString name, int v) {
  if (!inRange(player))
    return;
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueBool(int player, QString name, bool v) {
  if (!inRange(player))
    return;
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerTrigger(int player, QString name) {
  if (!inRange(player))
    return;
  cout << player << " name " << qPrintable(name) << endl;
}

void AudioModel::playerLoad(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (!inRange(player))
    return;
}


void AudioModel::masterSetValueDouble(QString name, double v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterSetValueInt(QString name, int v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterSetValueBool(QString name, bool v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterTrigger(QString name) {
  cout << "master name " << qPrintable(name) << endl;
}

void AudioModel::run(bool doit) {
  mAudioIO->run(doit);
  if (doit) {
    //XXX make this configurable
    try {
      mAudioIO->connectToPhysical(0,0);
      mAudioIO->connectToPhysical(1,1);
    } catch (...) {
      cerr << "couldn't connect to physical jack audio ports" << endl;
    }
  }
}

bool AudioModel::inRange(int player) {
  return player >= 0 && player < mNumPlayers;
}

void AudioModel::queue(djaudio::Command * cmd) {
  mMaster->scheduler()->execute(cmd);
}
