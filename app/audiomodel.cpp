#include "audiomodel.h"
#include "defines.hpp"
#include "player.hpp"
#include "command.hpp"
#include <QThread>
#include <QTimer>
#include <QHash>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using djaudio::AudioBuffer;
using djaudio::AudioBufferPtr;
using djaudio::BeatBuffer;
using djaudio::BeatBufferPtr;
using djaudio::Command;

namespace {
  const double done_scale = static_cast<double>(dj::one_scale);
  int to_int(double v) {
    return static_cast<int>(v * done_scale);
  }
  double to_double(int v) {
    return static_cast<double>(v) / done_scale;
  }
}

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

struct EnginePlayerState {
  unsigned int frame_current;
  unsigned int frame_count;
  double play_speed;
  float max_sample_value;
  bool audible;
};

struct PlayerState {
  QHash<QString, bool> boolValue;
  QHash<QString, int> intValue;
  QHash<QString, double> doubleValue;
};

AudioModel::AudioModel(QObject *parent) :
  QObject(parent)
{
  mAudioIO = djaudio::AudioIO::instance();
  mMaster  = djaudio::Master::instance();

  for(int i = 0; i < mNumPlayers; i++) {
    djaudio::Player * p = mMaster->add_player();
    PlayerState * pstate = new PlayerState;
    mPlayerStates.push_back(pstate);
    pstate->intValue["volume"] = to_int(p->volume());
    pstate->intValue["eq_high"] = 0;
    pstate->intValue["eq_mid"] = 0;
    pstate->intValue["eq_low"] = 0;

    pstate->boolValue["sync"] = p->syncing();
    pstate->boolValue["play"] = p->play_state() == djaudio::Player::PLAY;
    pstate->boolValue["cue"] = p->out_state() == djaudio::Player::CUE;
    pstate->boolValue["mute"] = p->muted();

    pstate->doubleValue["speed"] = p->play_speed();
  }

  mConsumeThread = new ConsumeThread(mMaster->scheduler(), this);
}

AudioModel::~AudioModel() {
  run(false);
}

void AudioModel::playerSetValueDouble(int player, QString name, double v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      return nullptr;
    });
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueInt(int player, QString name, int v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (pstate->intValue.contains(name) && pstate->intValue[name] == v)
        return nullptr;
      if (name == "volume")
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::VOLUME, to_double(v));
      if (cmd) {
        pstate->intValue[name] = v;
        emit(playerValueChangedInt(player, name, v));
      }
      return cmd;
    });
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerSetValueBool(int player, QString name, bool v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (pstate->boolValue.contains(name) && pstate->boolValue[name] == v)
        return nullptr;

      if (name == "cue")
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::OUT_CUE : djaudio::PlayerStateCommand::OUT_MAIN);
      else if (name == "play")
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::PLAY : djaudio::PlayerStateCommand::PAUSE);
      else if (name == "sync")
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::SYNC : djaudio::PlayerStateCommand::NO_SYNC);
      else if (name == "mute")
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::MUTE : djaudio::PlayerStateCommand::NO_MUTE);

      if (cmd) {
        pstate->boolValue[name] = v;
        emit(playerValueChangedBool(player, name, v));
      }
      return cmd;
    });
  cout << player << " name " << qPrintable(name) << v << endl;
}

void AudioModel::playerTrigger(int player, QString name) {
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

void AudioModel::playerClear(int player) {
  playerLoad(player, djaudio::AudioBufferPtr(), djaudio::BeatBufferPtr());
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

void AudioModel::playerSet(int player, std::function<djaudio::Command *(PlayerState * state)> func) {
  if (!inRange(player))
    return;
  Command * cmd = func(mPlayerStates[player]);
  if (cmd)
    queue(cmd);
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
