#include "audiomodel.h"
#include "defines.hpp"
#include "player.hpp"
#include "command.hpp"
#include "loopandjumpmanager.h"
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

using dj::to_int;
using dj::to_double;

namespace {
  //convert a *_relative into an absolute value if we have the non _relative verison
  //in our map
  template <typename T, typename V>
    void absoluteize(QString& name, V& v, T& map) {
      if (name.contains("_relative")) {
        QString non_rel_name = name;
        non_rel_name.remove("_relative");
        auto it = map.find(non_rel_name);
        if (it != map.end()) {
          name = non_rel_name;
          v += *it;
        }
      }
    }
}

class Consumer : public QObject {
  private:
    EngineQueryCommand * mQueryCmd = nullptr;
    djaudio::Scheduler * mScheduler = nullptr;
  public:
    Consumer(djaudio::Scheduler * scheduler, EngineQueryCommand * cmd, QObject * parent = nullptr) : 
      QObject(parent),
      mQueryCmd(cmd),
      mScheduler(scheduler) { }

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

  mLoopAndJumpManager = new LoopAndJumpManager(this);

  //broadcast the manager's updates, and hook it into us
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedInt(int, QString, int)), SIGNAL(playerValueChangedInt(int, QString, int))); 
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedInt(int, QString, int)), SLOT(playerSetValueInt(int, QString, int))); 
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedBool(int, QString, bool)), SIGNAL(playerValueChangedBool(int, QString, bool))); 
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedBool(int, QString, bool)), SLOT(playerSetValueBool(int, QString, bool))); 
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedDouble(int, QString, double)), SIGNAL(playerValueChangedDouble(int, QString, double))); 
  connect(mLoopAndJumpManager, SIGNAL(playerValueChangedDouble(int, QString, double)), SLOT(playerSetValueDouble(int, QString, double))); 

  connect(mLoopAndJumpManager, &LoopAndJumpManager::entryUpdated, this, &AudioModel::jumpUpdated);
  connect(mLoopAndJumpManager, &LoopAndJumpManager::entriesCleared, this, &AudioModel::jumpsCleared);
  connect(mLoopAndJumpManager, &LoopAndJumpManager::entryCleared, this, &AudioModel::jumpCleared);

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

    pstate->intValue["updates_since_sync"] = 0;

    pstate->boolValue["sync"] = p->syncing();
    pstate->boolValue["play"] = p->play_state() == djaudio::Player::PLAY;
    pstate->boolValue["cue"] = p->out_state() == djaudio::Player::CUE;
    pstate->boolValue["mute"] = p->muted();
    pstate->boolValue["audible"] = false;
    pstate->boolValue["loop"] = false;

    pstate->doubleValue["speed"] = p->play_speed() - 1.0;
    pstate->doubleValue["loop_length_beats"] = 0;
  }

  mMasterDoubleValue["bpm"] = mMaster->transport()->bpm();
  mMasterIntValue["volume"] = to_int(mMaster->master_volume());
  mMasterIntValue["cue_volume"] = to_int(mMaster->cue_volume());

  EngineQueryCommand * query = new EngineQueryCommand(mNumPlayers);
  mConsumeThread = new QThread(this);
  mConsumer = new Consumer(mMaster->scheduler(), query);
  mConsumer->moveToThread(mConsumeThread);

  QTimer * consumetimer = new QTimer(this);
  //XXX if the UI becomes unresponsive, increase this value
  consumetimer->setInterval(15);

  connect(consumetimer, &QTimer::timeout, mConsumer, &Consumer::grabCommands);
  connect(mConsumeThread, &QThread::started, consumetimer, static_cast<void (QTimer::*)(void)>(&QTimer::start));
  connect(mConsumeThread, &QThread::finished, consumetimer, &QTimer::stop);

  connect(query, &EngineQueryCommand::playerValueUpdateBool, this, &AudioModel::playerSetValueBool);
  connect(query, &EngineQueryCommand::playerValueUpdateInt, this, &AudioModel::playerSetValueInt);
  connect(query, &EngineQueryCommand::playerValueUpdateDouble, this, &AudioModel::playerSetValueDouble);

  connect(query, &EngineQueryCommand::masterValueUpdateDouble, this, &AudioModel::masterSetValueDouble);
}

AudioModel::~AudioModel() {
  prepareToQuit();
}

void AudioModel::createClient(QString name){
  mAudioIO->createClient(name.toStdString());
}

void AudioModel::setDB(DB * db) {
  mLoopAndJumpManager->setDB(db);
}

void AudioModel::prepareToQuit() {
  mLoopAndJumpManager->saveData();
  run(false);
}

void AudioModel::playerSetValueDouble(int player, QString name, double v) {
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      //relay from Consumer
      //we want to always relay it, no matter what the last value was
      if (name == "audio_level") {
        pstate->doubleValue[name] = v;
        emit(playerValueChangedDouble(player, name, v));
        return nullptr;
      } else if (name == "loop_length_beats") {
        bool was_looping = pstate->boolValue["loop"];
        double loop_beats_last = pstate->doubleValue["loop_length_beats"];
        pstate->boolValue["loop"] = true;
        PlayerLoopAndReportCommand * c = new PlayerLoopAndReportCommand(player, v);
        //relay changes
        connect(c, &PlayerLoopAndReportCommand::playerValueChangedBool, this, &AudioModel::playerSetValueBool);
        connect(c, &PlayerLoopAndReportCommand::playerValueChangedInt, this, &AudioModel::playerSetValueInt);
        connect(c, &PlayerLoopAndReportCommand::playerValueChangedInt, mLoopAndJumpManager, &LoopAndJumpManager::playerSetValueInt);

        //if we're growing an active synced loop from less than a beat length to longer, wait until the beat boundary to execute the new length
        if (was_looping && loop_beats_last < 1.0 && loop_beats_last < v && pstate->boolValue["sync"]) {
          cmd = new djaudio::MasterNextBeatCommand(c);
          return cmd;
        }

        return c;
      }

      auto it = pstate->doubleValue.find(name);
      if (it != pstate->doubleValue.end() && *it == v)
        return nullptr;

      absoluteize(name, v, pstate->doubleValue);
      if (name == "speed") {
        if (!pstate->boolValue["sync"])
          cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::PLAY_SPEED, 1.0 + v / 100.0);
      } else if (name == "update_speed") {
        if (pstate->boolValue["sync"] && pstate->intValue["updates_since_sync"] > 2 && pstate->doubleValue["speed"] != v) {
          emit(playerValueChangedDouble(player, "speed", v));
          pstate->doubleValue["speed"] = v;
        }
        return nullptr;
      }
      if (cmd) {
        pstate->doubleValue[name] = v;
        emit(playerValueChangedDouble(player, name, v));
      }
      return cmd;
    });
  //cout << player <<  qPrintable(name) << " " << v << endl;
}

void AudioModel::playerSetValueInt(int player, QString name, int v) {
  mLoopAndJumpManager->playerSetValueInt(player, name, v);
  playerSet(player, [player, &name, &v, this](PlayerState * pstate) -> Command *
    {
      if (name == "seek_frame_relative") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_RELATIVE, v);
      } else if (name == "seek_beat_relative") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT_RELATIVE, v);
      }

      djaudio::Command * cmd = nullptr;
      auto it = pstate->intValue.find(name);
      if (it != pstate->intValue.end() && *it == v)
        return nullptr;

      absoluteize(name, v, pstate->intValue);
      //just return when we don't want to report
      if (name == "volume") {
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::VOLUME, to_double(v));
      } else if (name == "seek_frame") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY, v < 0 ? 0 : v);
      } else if (name == "seek_beat") {
        return new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT, v < 0 ? 0 : v);
      } else if (name == "eq_high") {
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::EQ_HIGH, to_double(v));
      } else if (name == "eq_mid") {
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::EQ_MID, to_double(v));
      } else if (name == "eq_low") {
        cmd = new djaudio::PlayerDoubleCommand(player, djaudio::PlayerDoubleCommand::EQ_LOW, to_double(v));
      } else if (name == "loop_start_frame") {
        cmd = new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::LOOP_START, v);
      } else if (name == "loop_end_frame") {
        cmd = new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::LOOP_END, v);
      } else if (name == "position_frame") {
        pstate->intValue["updates_since_sync"] += 1;
        emit (playerValueChangedInt(player, name, v)); //relaying from Consumer
        pstate->intValue[name] = v;
        return nullptr;
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
      auto it = pstate->boolValue.find(name);
      if (it != pstate->boolValue.end() && *it == v)
        return nullptr;

      if (name == "cue") {
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::OUT_CUE : djaudio::PlayerStateCommand::OUT_MAIN);
      } else if (name == "play") {
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::PLAY : djaudio::PlayerStateCommand::PAUSE);
      } else if (name == "sync") {
        pstate->intValue["updates_since_sync"] = 0;
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::SYNC : djaudio::PlayerStateCommand::NO_SYNC);
      } else if (name == "bump_fwd_down") {
        if (!pstate->boolValue["sync"]) {
          cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::BUMP_FWD : djaudio::PlayerStateCommand::BUMP_OFF);
        } else {
          if (v)
            playerTrigger(player, "seek_fwd");
        }
      } else if (name == "bump_back_down") {
        if (!pstate->boolValue["sync"]) {
          cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::BUMP_REV : djaudio::PlayerStateCommand::BUMP_OFF);
        } else {
          if (v)
            playerTrigger(player, "seek_back");
        }
      } else if (name == "mute") {
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::MUTE : djaudio::PlayerStateCommand::NO_MUTE);
      } else if (name == "seeking") {
        pstate->boolValue["seeking"] = v;
        emit(playerValueChangedBool(player, name, v));
        //if we are already paused, don't do anything
        if (!pstate->boolValue["play"])
          return nullptr;
        return new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::PAUSE : djaudio::PlayerStateCommand::PLAY);
      } else if (name == "audible") {
        pstate->boolValue[name] = v; //relaying from Consumer
        emit(playerValueChangedBool(player, name, v));
        return nullptr;
      } else if (name == "loop") {
        cmd = new djaudio::PlayerStateCommand(player, v ? djaudio::PlayerStateCommand::LOOP : djaudio::PlayerStateCommand::NO_LOOP);
        //if we're stopping looping a less than 1 beat loop and trying to stay in sync we better wait till the next beat
        if (!v && pstate->boolValue["sync"] && pstate->doubleValue["loop_length_beats"] < 1.0) {
          cmd = new djaudio::MasterNextBeatCommand(cmd);
        }
      } 

      if (cmd) {
        pstate->boolValue[name] = v;
        emit(playerValueChangedBool(player, name, v));
      }
      return cmd;
    });
  if (name != "audible")
    cout << player << qPrintable(name) << " " << v << endl;
}

void AudioModel::playerTrigger(int player, QString name) {
  if (!inRange(player))
    return;
  mLoopAndJumpManager->playerTrigger(player, name);

  bool triggered = false;
  playerSet(player, [player, &name, &triggered, this](PlayerState * /*pstate*/) -> Command *
    {
      djaudio::Command * cmd = nullptr;
      if (name.contains("seek_")) {
        cmd = new djaudio::PlayerPositionCommand(player, djaudio::PlayerPositionCommand::PLAY_BEAT_RELATIVE, 
          (name == "seek_fwd" ? 1 : -1));
      }
      triggered = cmd;
      return cmd;
    });
  //cout << player << qPrintable(name) << endl;
  
  //if we didn't have a trigger, see if we have a bool value for it
  if (!triggered) {
    auto it = mPlayerStates[player]->boolValue.find(name);
    if (it != mPlayerStates[player]->boolValue.end())
      playerSetValueBool(player, name, !*it);
  }
}

void AudioModel::playerLoad(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (!inRange(player))
    return;
  mLoopAndJumpManager->playerLoad(player, audio_buffer, beat_buffer);

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
  //we always want to send the audio level along
  if (name == "audio_level") {
    emit(masterValueChangedDouble(name, v));
    return;
  }

  auto it = mMasterDoubleValue.find(name);
  if (it != mMasterDoubleValue.end() && *it == v)
    return;

  absoluteize(name, v, mMasterDoubleValue);
  if (name == "bpm") {
    queue(new djaudio::TransportBPMCommand(mMaster->transport(), v));
  } else if (name == "update_bpm") {
    if (mMasterDoubleValue["bpm"] != v) {
      mMasterDoubleValue["bpm"] = v;
      emit(masterValueChangedDouble("bpm", v));
    }
    return; 
  } else {
    cout << "master name " << qPrintable(name) << v << endl;
    return;
  }
  mMasterDoubleValue[name] = v;
  emit(masterValueChangedDouble(name, v));
}

void AudioModel::masterSetValueInt(QString name, int v) {
  if (name == "sync_to_player") {
    MasterSyncToPlayerCommand * cmd = new MasterSyncToPlayerCommand(v);
    queue(cmd);
    QObject::connect(
        cmd, &MasterSyncToPlayerCommand::masterValueUpdateDouble,
        this, &AudioModel::masterSetValueDouble);
    return;
  }

  auto it = mMasterIntValue.find(name);
  if (it != mMasterIntValue.end() && *it == v)
    return;

  absoluteize(name, v, mMasterIntValue);
  if (name == "volume") {
    queue(new djaudio::MasterDoubleCommand(djaudio::MasterDoubleCommand::MAIN_VOLUME, to_double(v)));
  } else if (name == "cue_volume") {
    queue(new djaudio::MasterDoubleCommand(djaudio::MasterDoubleCommand::CUE_VOLUME, to_double(v)));
  } else {
    cout << "master name " << qPrintable(name) << v << endl;
    return;
  }
  mMasterIntValue[name] = v;
  emit(masterValueChangedInt(name, v));
}

void AudioModel::masterSetValueBool(QString name, bool v) {
  cout << "master name " << qPrintable(name) << v << endl;
}

void AudioModel::masterTrigger(QString name) {
  if (name.contains("sync_to_player")) {
    int player = name.remove("sync_to_player").toInt();
    if (!inRange(player))
      return;
    masterSetValueInt("sync_to_player", player);
    PlayerState * pstate = mPlayerStates[player];
    if (!pstate->boolValue["sync"]) {
      pstate->boolValue["sync"] = true;
      emit(playerValueChangedBool(player, "sync", true));
    }
    return;
  }
  cout << "master name " << qPrintable(name) << endl;
}

void AudioModel::pluginSetValueInt(int plugin_index, QString parameter_name, int value) {
  auto it = mPlugins.find(plugin_index);
  if (it == mPlugins.end())
    return;
  //XXX do it
}

void AudioModel::pluginAddToPlayer(int player_index, int location_index, AudioPluginPtr plugin) {
  mPlugins[plugin->index()] = plugin;
  //XXX do it
}

void AudioModel::pluginAddToMaster(int send_index, int location_index, AudioPluginPtr plugin) {
  mPlugins[plugin->index()] = plugin;
  //XXX do it
}

void AudioModel::pluginRemove(int plugin_index) {
  auto it = mPlugins.find(plugin_index);
  if (it != mPlugins.end()) {
    mPluginsToDelete << *it;
    mPlugins.erase(it);
  }
  //XXX do it!
}

void AudioModel::run(bool doit) {
  mAudioIO->run(doit);
  if (doit) {
    mConsumeThread->start();
#if 0
    //XXX make this configurable
    try {
      mAudioIO->connectToPhysical(0,0);
      mAudioIO->connectToPhysical(1,1);
    } catch (...) {
      cerr << "couldn't connect to physical jack audio ports" << endl;
    }
#endif
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

void PlayerSetBuffersCommand::execute(const djaudio::Transport& /*transport*/) {
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

PlayerLoopAndReportCommand::PlayerLoopAndReportCommand(unsigned int idx, double beats, PlayerLoopCommand::resize_policy_t resize_policy, bool start_looping) : PlayerLoopCommand(idx, beats, resize_policy, start_looping) {
}

PlayerLoopAndReportCommand::PlayerLoopAndReportCommand(unsigned int idx, long start_frame, long end_frame, bool start_looping) : PlayerLoopCommand(idx, start_frame, end_frame, start_looping) {
}

PlayerLoopAndReportCommand::~PlayerLoopAndReportCommand() {
}

void PlayerLoopAndReportCommand::execute_done() {
  int pindex = index();

  emit(playerValueChangedBool(pindex, "looping", looping()));
  emit(playerValueChangedInt(pindex, "loop_start_frame", static_cast<int>(start_frame())));
  emit(playerValueChangedInt(pindex, "loop_end_frame", static_cast<int>(end_frame())));
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

void EngineQueryCommand::execute(const djaudio::Transport& /*transport*/) {
  djaudio::Master * m = master();
  for (size_t i = 0; i < std::min(m->players().size(), (size_t)mPlayerStates.size()); i++) {
    djaudio::Player * p = m->players().at(i);
    EnginePlayerState * ps = mPlayerStates[i];

    ps->play_speed = p->play_speed();
    ps->max_sample_value = p->max_sample_value_reset();
    ps->audible = p->audible();
    ps->frame_current = p->frame();
  }
  mMasterVolume = m->max_sample_value_reset();
  mMasterBPM = m->transport()->bpm();
}

void EngineQueryCommand::execute_done() {
  for (int i = 0; i < mPlayerStates.size(); i++) {
    EnginePlayerState * ps = mPlayerStates[i];
    emit(playerValueUpdateDouble(i, "update_speed", (ps->play_speed - 1.0) * 100.0));
    emit(playerValueUpdateDouble(i, "audio_level", ps->max_sample_value));
    emit(playerValueUpdateInt(i, "position_frame", ps->frame_current));
    emit(playerValueUpdateBool(i, "audible", ps->audible));
  }
  //emit(masterValueUpdateDouble("update_bpm", mMasterBPM));
  emit(masterValueUpdateDouble("audio_level", mMasterVolume));
}

bool EngineQueryCommand::store(djaudio::CommandIOData& /* data */) const { return false; }

MasterSyncToPlayerCommand::MasterSyncToPlayerCommand(int value) :
  QObject(NULL),
  djaudio::MasterIntCommand(djaudio::MasterIntCommand::SYNC_TO_PLAYER, value), mBPM(0.0) {
  }

void MasterSyncToPlayerCommand::execute(const djaudio::Transport& transport) {
  //execute the normal command then grab the bpm
  djaudio::MasterIntCommand::execute(transport);
  mBPM = master()->transport()->bpm();
}

void MasterSyncToPlayerCommand::execute_done() {
  emit(masterValueUpdateDouble("update_bpm", mBPM));
}
