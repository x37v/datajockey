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
    EngineQueryCommand * mQueryCmd = nullptr;
    djaudio::Scheduler * mScheduler = nullptr;
    QTimer * mTimer = nullptr;
  public:
    ConsumeThread(djaudio::Scheduler * scheduler, EngineQueryCommand * cmd, QObject * parent) : 
      QThread(parent),
      mQueryCmd(cmd),
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
        EngineQueryCommand * query = dynamic_cast<EngineQueryCommand*>(cmd);
        if (query) {
          mQueryCmd = query;
        } else if (cmd) {
          delete cmd;
        }
      }
      if (mQueryCmd) {
        mScheduler->execute(mQueryCmd);
        mQueryCmd = nullptr;
      }
    }

    void run() {
      mTimer->start();
      exec();
    }
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
    p->sync(true);
    p->play_state(djaudio::Player::PLAY);

    PlayerState * pstate = new PlayerState;
    mPlayerStates.push_back(pstate);
    pstate->intValue["volume"] = to_int(p->volume());
    pstate->intValue["eq_high"] = 0;
    pstate->intValue["eq_mid"] = 0;
    pstate->intValue["eq_low"] = 0;

    pstate->intValue["frames"] = 0;
    pstate->intValue["position_frame"] = 0;
    pstate->intValue["sample_rate"] = 44100;

    pstate->boolValue["sync"] = p->syncing();
    pstate->boolValue["play"] = p->play_state() == djaudio::Player::PLAY;
    pstate->boolValue["cue"] = p->out_state() == djaudio::Player::CUE;
    pstate->boolValue["mute"] = p->muted();
    pstate->boolValue["audible"] = false;

    pstate->doubleValue["speed"] = 1.0 + p->play_speed();
  }

  EngineQueryCommand * query = new EngineQueryCommand(mNumPlayers);
  mConsumeThread = new ConsumeThread(mMaster->scheduler(), query, this);

  //XXX lambdas cannot have queued connections!
  connect(query, &EngineQueryCommand::playerValueUpdateBool,
      [this](int player, QString name, bool value) {
        if (!inRange(player))
          return;
        PlayerState * pstate = mPlayerStates[player];
        if (pstate->boolValue.contains(name) && pstate->boolValue[name] == value)
          return;
        if (name == "audible")
          emit (playerValueChangedBool(player, name, value));
        pstate->boolValue[name] = value;
      });

  connect(query, &EngineQueryCommand::playerValueUpdateInt,
      [this](int player, QString name, int value) {
        if (!inRange(player))
          return;
        PlayerState * pstate = mPlayerStates[player];
        if (pstate->intValue.contains(name) && pstate->intValue[name] == value)
          return;
        if (name == "position_frame")
          emit (playerValueChangedInt(player, name, value));
        pstate->intValue[name] = value;
      });
  connect(query, &EngineQueryCommand::playerValueUpdateDouble,
      [this](int player, QString name, double value) {
        if (!inRange(player))
          return;
        PlayerState * pstate = mPlayerStates[player];
        if (pstate->doubleValue.contains(name) && pstate->doubleValue[name] == value)
          return;
        if (name == "audio_level")
          emit (playerValueChangedDouble(player, name, value));
        //XXX deal with speed
        pstate->doubleValue[name] = value;
      });
}

AudioModel::~AudioModel() {
  run(false);
}

void AudioModel::playerSetValueDouble(int player, QString name, double v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (pstate->doubleValue.contains(name) && pstate->doubleValue[name] == v)
        return nullptr;
      if (name == "speed")
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::PLAY_SPEED, 1.0 + v / 100.0);
      if (cmd) {
        pstate->doubleValue[name] = v;
        emit(playerValueChangedDouble(player, name, v));
      }
      return cmd;
    });
  //cout << player <<  qPrintable(name) << " " << v << endl;
}

void AudioModel::playerSetValueInt(int player, QString name, int v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (pstate->intValue.contains(name) && pstate->intValue[name] == v)
        return nullptr;
      
      //just return when we don't want to report
      if (name == "volume") {
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::VOLUME, to_double(v));
      } else if (name == "seek_frame") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY, v < 0 ? 0 : v);
      } else if (name == "seek_beat") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT, v < 0 ? 0 : v);
      } else if (name == "seek_frame_relative") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_RELATIVE, v);
      } else if (name == "seek_beat_relative") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT_RELATIVE, v);
      }

      if (cmd) {
        pstate->intValue[name] = v;
        emit(playerValueChangedInt(player, name, v));
      }
      return cmd;
    });
  //cout << player << qPrintable(name) << " " << v << endl;
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
      else if (name == "seeking") {
        pstate->boolValue["seeking"] = v;
        emit(playerValueChangedBool(player, name, v));
        //if we are already paused, don't do anything
        if (!pstate->boolValue["play"])
          return nullptr;
        return new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::PAUSE : djaudio::PlayerStateCommand::PLAY);
      }

      if (cmd) {
        pstate->boolValue[name] = v;
        emit(playerValueChangedBool(player, name, v));
      }
      return cmd;
    });
  //cout << player << qPrintable(name) << " " << v << endl;
}

void AudioModel::playerTrigger(int player, QString name) {
  playerSet(player, [player, &name, this](PlayerState * /*pstate*/) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (name.contains("seek_")) {
        cmd = new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT_RELATIVE, 
          (name == "seek_fwd" ? 1 : -1));
      }
      return cmd;
    });
  //cout << player << qPrintable(name) << endl;
}

void AudioModel::playerLoad(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  cout << "playerLoad: " << player << endl;
  if (!inRange(player))
    return;

  PlayerState * pstate = mPlayerStates[player];
  pstate->intValue["frames"] = audio_buffer ? audio_buffer->length() : 0;
  pstate->intValue["sample_rate"] = audio_buffer ? audio_buffer->sample_rate() : 44100;

  //store a copy
  if (audio_buffer)
    mAudioBuffers.push_back(audio_buffer);
  if (beat_buffer)
    mBeatBuffers.push_back(beat_buffer);

  if (mCueOnLoad && audio_buffer) {
    for (int i = 0; i < mNumPlayers; i++)
      playerSetValueBool(i, "cue", i == player);
  }

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
  auto it = mMasterDoubleValue.find(name);
  if (it != mMasterDoubleValue.end() && *it == v)
    return;
  if (name == "bpm")
    queue(new djaudio::TransportBPMCommand(mMaster->transport(), v));
  else {
    cout << "master name " << qPrintable(name) << v << endl;
    return;
  }
  mMasterDoubleValue[name] = v;
}

void AudioModel::masterSetValueInt(QString name, int v) {
  auto it = mMasterIntValue.find(name);
  if (it != mMasterIntValue.end() && *it == v)
    return;
  if (name == "volume")
    queue(new djaudio::MasterDoubleCommand(djaudio::MasterDoubleCommand::MAIN_VOLUME, to_double(v)));
  else if (name == "cue_volume")
    queue(new djaudio::MasterDoubleCommand(djaudio::MasterDoubleCommand::CUE_VOLUME, to_double(v)));
  else {
    cout << "master name " << qPrintable(name) << v << endl;
    return;
  }
  mMasterIntValue[name] = v;
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

void AudioModel::masterSet(std::function<djaudio::Command *(void)> func) {
  Command * cmd = func();
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

struct EnginePlayerState {
  unsigned int frame_current;
  double play_speed;
  float max_sample_value;
  bool audible;
};

EngineQueryCommand::EngineQueryCommand(int num_players, QObject * parent) : QObject(parent), djaudio::MasterCommand()
{
  for (int i = 0; i < num_players; i++)
    mPlayerStates.push_back(new EnginePlayerState);
}

bool EngineQueryCommand::delete_after_done() { return false; }

void EngineQueryCommand::execute() {
  djaudio::Master * m = master();
  for (size_t i = 0; i < std::min(m->players().size(), (size_t)mPlayerStates.size()); i++) {
    djaudio::Player * p = m->players().at(i);
    EnginePlayerState * ps = mPlayerStates[i];

    ps->play_speed = p->play_speed();
    ps->max_sample_value = p->max_sample_value();
    ps->audible = p->audible();
    ps->frame_current = p->frame();

    p->max_sample_value_reset();
  }
}

void EngineQueryCommand::execute_done() {
  for (int i = 0; i < mPlayerStates.size(); i++) {
    EnginePlayerState * ps = mPlayerStates[i];
    emit(playerValueUpdateDouble(i, "speed", (ps->play_speed - 1.0) * 100.0));
    emit(playerValueUpdateDouble(i, "audio_level", ps->max_sample_value));
    emit(playerValueUpdateInt(i, "position_frame", ps->frame_current));
    emit(playerValueUpdateBool(i, "audible", ps->audible));
  }
  emit(masterValueUpdateDouble("bpm", mMasterBPM));
  emit(masterValueUpdateDouble("audio_level", mMasterVolume));
}

bool EngineQueryCommand::store(djaudio::CommandIOData& /* data */) const { return false; }

