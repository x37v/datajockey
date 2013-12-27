#include "audiomodel.h"
#include "defines.hpp"
#include "player.hpp"
#include "command.hpp"
#include <QThread>
#include <QTimer>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using djaudio::AudioBuffer;
using djaudio::AudioBufferPtr;
using djaudio::BeatBuffer;
using djaudio::BeatBufferPtr;

class ConsumeThread : public QThread {
  private:
    djaudio::Scheduler * mScheduler;
    QTimer * mTimer;
    //QueryPlayState * mQueryCmd;
  public:
    ConsumeThread(djaudio::Scheduler * scheduler, QObject * parent) : 
      QThread(parent),
      mScheduler(scheduler)
  {
    mTimer = new QTimer(this);
    //XXX if the UI becomes unresponsive, increase this value
    mTimer->setInterval(15);

    connect(mTimer, &QTimer::timeout, this, &ConsumeThread::grabCommands);
    connect(this, &QThread::finished, mTimer, &QTimer::stop);
  }

    void grabCommands() {
      mScheduler->execute_done_actions();
      while (djaudio::Command * cmd = mScheduler->pop_complete_command()) {
        if (cmd)
          delete cmd;
      }
    }

    void run() {
      mTimer->start();
      exec();
    }
};

AudioModel::AudioModel(QObject *parent) :
  QObject(parent)
{
  mAudioIO = djaudio::AudioIO::instance();
  mMaster  = djaudio::Master::instance();

  for(int i = 0; i < mNumPlayers; i++) {
    mMaster->add_player();
  }

  mConsumeThread = new ConsumeThread(mMaster->scheduler(), this);
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

  //store a copy
  if (audio_buffer)
    mAudioBuffers.push_back(audio_buffer);
  if (beat_buffer)
    mBeatBuffers.push_back(beat_buffer);

  PlayerSetBuffersCommand * cmd = new PlayerSetBuffersCommand(player, audio_buffer.data(), beat_buffer.data());
  connect(cmd, &PlayerSetBuffersCommand::done,
      [this](AudioBufferPtr ab, BeatBufferPtr bb) {
        mAudioBuffers.removeOne(ab);
        mBeatBuffers.removeOne(bb);
      });
  queue(cmd);
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
    mConsumeThread->start();
    //XXX make this configurable
    try {
      mAudioIO->connectToPhysical(0,0);
      mAudioIO->connectToPhysical(1,1);
    } catch (...) {
      cerr << "couldn't connect to physical jack audio ports" << endl;
    }
  } else {
    mConsumeThread->quit();
  }
}

bool AudioModel::inRange(int player) {
  return player >= 0 && player < mNumPlayers;
}

void AudioModel::queue(djaudio::Command * cmd) {
  mMaster->scheduler()->execute(cmd);
}


PlayerSetBuffersCommand::PlayerSetBuffersCommand(unsigned int idx,
    AudioBuffer * audio_buffer, BeatBuffer * beat_buffer) :
  QObject(nullptr),
  djaudio::PlayerCommand(idx),
  mAudioBuffer(audio_buffer),
  mBeatBuffer(beat_buffer),
  mOldAudioBuffer(nullptr),
  mOldBeatBuffer(nullptr)
{ }

PlayerSetBuffersCommand::~PlayerSetBuffersCommand() { }

void PlayerSetBuffersCommand::execute() {
  djaudio::Player * p = player(); 
  if(p != NULL){
    mOldBeatBuffer = p->beat_buffer();
    mOldAudioBuffer = p->audio_buffer();
    p->audio_buffer(mAudioBuffer);
    p->beat_buffer(mBeatBuffer);
    p->play_state(djaudio::Player::PLAY);
    p->volume(1.0);
    p->sync(true);
  }
}

void PlayerSetBuffersCommand::execute_done() {
  AudioBufferPtr audio_buffer(mOldAudioBuffer);
  BeatBufferPtr beat_buffer(mOldBeatBuffer);
  emit(done(audio_buffer, beat_buffer));
  //execute the super class's done action
  PlayerCommand::execute_done();
}

bool PlayerSetBuffersCommand::store(djaudio::CommandIOData& data) const {
  PlayerCommand::store(data, "PlayerSetBuffersCommand");
  //TODO
  return false;
}
