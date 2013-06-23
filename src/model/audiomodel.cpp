#include "audiomodel.hpp"
#include "loaderthread.hpp"
#include "transport.hpp"
#include "defines.hpp"

#include <vector>
#include <QMetaObject>
#include <QMetaType>
#include <QTimer>

using namespace dj;
using namespace dj::audio;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

namespace {
  const int one_point_five = static_cast<int>((double)one_scale * 1.5);
  const int ione_scale = one_scale;

  QList<int> player_loop_beats = {1, 2, 4, 8, 16};

  void init_signal_hashes() {
    QStringList list;

    //*** PLAYER

    list << "load" << "reset" << "seek_forward" << "seek_back" << "bump_forward" << "bump_back" << "loop" << "loop_off" << "loop_shift_back" << "loop_shift_forward";
    for (int i = 0; i < 10; i++) {
      list << QString("set_cuepoint_%1").arg(i);
      list << QString("jump_cuepoint_%1").arg(i);
    }

    AudioModel::player_signals["trigger"] = list;

    list.clear();
    list << "cue" << "sync" << "pause" << "loop";
    AudioModel::player_signals["bool"] = list;

    list.clear();
    list << "volume" << "speed" << "eq_low" << "eq_mid" << "eq_high" << "play_frame" << "play_beat";
    AudioModel::player_signals["int"] = list;

    list.clear();
    list << "play_position";
    AudioModel::player_signals["double"] = list;

    //*** MASTER

    list.clear();
    list << "sync_to_player0";
    list << "sync_to_player1";
    AudioModel::master_signals["trigger"] = list;

    list.clear();
    list << "crossfade_enabled";
    AudioModel::master_signals["bool"] = list;

    list.clear();
    list << "volume" << "crossfade_position" << "crossfade_player_left" << "crossfade_player_right" << "track";
    AudioModel::master_signals["int"] = list;

    list.clear();
    list << "bpm";
    AudioModel::master_signals["double"] = list;


    //set up relative
    QStringList list_with_rel;
    QString signal;

    list_with_rel.clear();
    list = AudioModel::player_signals["int"];
    foreach(signal, list) {
      list_with_rel << signal << (signal + "_relative");
    }
    list_with_rel << "loop_beats" << "loop_shift";
    AudioModel::player_signals["int"] = list_with_rel;

    list_with_rel.clear();
    list = AudioModel::player_signals["double"];
    foreach(signal, list) {
      list_with_rel << signal << (signal + "_relative");
    }
    AudioModel::player_signals["double"] = list_with_rel;

    list = AudioModel::master_signals["int"];
    foreach(signal, list) {
      list_with_rel << signal << (signal + "_relative");
    }
    AudioModel::master_signals["int"] = list_with_rel;

    list_with_rel.clear();
    list = AudioModel::master_signals["double"];
    foreach(signal, list) {
      list_with_rel << signal << (signal + "_relative");
    }
    AudioModel::master_signals["double"] = list_with_rel;
  }
}


QHash<QString, QStringList> AudioModel::player_signals;
QHash<QString, QStringList> AudioModel::master_signals;

QList<int> AudioModel::loop_beats() {
  return player_loop_beats;
}

class AudioModel::PlayerSetBuffersCommand : public dj::audio::PlayerCommand {
  public:
    PlayerSetBuffersCommand(unsigned int idx,
        AudioModel * model,
        AudioBuffer * audio_buffer,
        BeatBuffer * beat_buffer
        ) :
      dj::audio::PlayerCommand(idx),
      mAudioModel(model),
      mAudioBuffer(audio_buffer),
      mBeatBuffer(beat_buffer),
      mOldAudioBuffer(NULL),
      mOldBeatBuffer(NULL) { }
    virtual ~PlayerSetBuffersCommand() { }

    virtual void execute() {
      Player * p = player(); 
      if(p != NULL){
        mOldBeatBuffer = p->beat_buffer();
        mOldAudioBuffer = p->audio_buffer();
        p->audio_buffer(mAudioBuffer);
        p->beat_buffer(mBeatBuffer);
      }
    }
    virtual void execute_done() {
      //remove a copy of the old buffers from the list
      if (mOldAudioBuffer) {
        AudioBufferPtr buffer(mOldAudioBuffer);
        mAudioModel->mPlayingAudioFiles.removeOne(buffer);
      }
      if (mOldBeatBuffer) {
        BeatBufferPtr buffer(mOldBeatBuffer);
        mAudioModel->mPlayingAnnotationFiles[index()].removeOne(buffer);
      }
      //execute the super class's done action
      PlayerCommand::execute_done();
    }
    virtual bool store(CommandIOData& data) const {
      PlayerCommand::store(data, "PlayerSetBuffersCommand");
      //TODO
      return false;
    }
  private:
    AudioModel * mAudioModel;
    AudioBuffer * mAudioBuffer;
    BeatBuffer * mBeatBuffer;
    AudioBuffer * mOldAudioBuffer;
    BeatBuffer * mOldBeatBuffer;
};

class AudioModel::PlayerState {
  public:
    PlayerState() :
      mCurrentFrame(0),
      mNumFrames(0),
      mSpeed(0.0),
      mPostFreeSpeedUpdates(0),
      mMaxSampleValue(0.0),
      mAudible(false),
      mLoopBeats(4)
  { }

    QHash<QString, bool> mParamBool;
    QHash<QString, int> mParamInt;
    QHash<QString, double> mParamDouble;
    QHash<QString, TimePoint> mParamPosition;

    //okay to update in audio thread
    unsigned int mCurrentFrame;
    unsigned int mNumFrames;
    double mSpeed;
    //we count the number of speed updates so that after going 'free' we get at least two updates to the gui
    unsigned int mPostFreeSpeedUpdates;
    float mMaxSampleValue;
    bool mAudible;
    unsigned int mLoopBeats;
};

class AudioModel::ConsumeThread : public QThread {
  private:
    Scheduler * mScheduler;
    QueryPlayState * mQueryCmd;
  public:
    ConsumeThread(QueryPlayState * query_cmd, Scheduler * scheduler) : mScheduler(scheduler), mQueryCmd(query_cmd) { }

    void run() {
      while(true) {
        if (mQueryCmd) {
          mScheduler->execute(mQueryCmd);
          mQueryCmd = NULL;
          msleep(5); //should give the command several execute cycles to get through
        }

        mScheduler->execute_done_actions();
        dj::audio::Command * cmd = mScheduler->pop_complete_command();
        //if we got a command and the dynamic cast fails, delete the command
        if (cmd && (mQueryCmd = dynamic_cast<QueryPlayState *>(cmd)) == NULL)
          delete cmd;

        //XXX if the UI becomes unresponsive, increase this value
        msleep(15);
      }
    }
};

AudioModel * AudioModel::cInstance = NULL;

AudioModel::AudioModel() :
  QObject(),
  mPlayerStates(),
  mPlayerAudibleThresholdVolume(0.05 * one_scale), //XXX make this configurable?
  mCrossfadeAudibleThresholdPosition(0.05 * one_scale),
  mBumpSeconds(0.25),
  mCueOnLoad(true)
{
  unsigned int num_players = 2;

  init_signal_hashes();

  //register signal types
  qRegisterMetaType<TimePoint>("TimePoint");
  qRegisterMetaType<AudioBufferPtr>("AudioBufferPtr");
  qRegisterMetaType<BeatBufferPtr>("BeatBufferPtr");

  //set them to be out of range at first, this will be updated later via a slot
  mMasterParamDouble["bpm"] = 10.0;
  mMasterParamBool["crossfade_enabled"] = false;
  mMasterParamInt["volume"] = 0;
  mMasterParamInt["cue_volume"] = 0;
  mMasterParamInt["crossfade_position"] = 0;
  mMasterParamInt["crossfade_player_left"] = num_players;
  mMasterParamInt["crossfade_player_right"] = num_players;
  mAudioIO = dj::audio::AudioIO::instance();
  mMaster = dj::audio::Master::instance();

  mNumPlayers = num_players;
  for(unsigned int i = 0; i < mNumPlayers; i++) {
    mMaster->add_player();
    mPlayerStates.push_back(new PlayerState());
    mPlayerCuepoints << QHash<int, unsigned int>();
    mPlayingAnnotationFiles << QList<BeatBufferPtr>();
    LoaderThread * newThread = new LoaderThread;
    mThreadPool.push_back(newThread);
    QObject::connect(newThread, SIGNAL(load_progress(int, int)),
        SLOT(relay_audio_file_load_progress(int, int)),
        Qt::QueuedConnection);
    QObject::connect(newThread, SIGNAL(load_complete(int, AudioBufferPtr, BeatBufferPtr)),
        SLOT(relay_player_buffers_loaded(int, AudioBufferPtr, BeatBufferPtr)),
        Qt::QueuedConnection);
  }

  //set up the bool action mappings
  mPlayerStateActionMapping["mute"] = player_onoff_action_pair_t(PlayerStateCommand::MUTE, PlayerStateCommand::NO_MUTE);
  mPlayerStateActionMapping["sync"] = player_onoff_action_pair_t(PlayerStateCommand::SYNC, PlayerStateCommand::NO_SYNC);
  mPlayerStateActionMapping["cue"] = player_onoff_action_pair_t(PlayerStateCommand::OUT_CUE, PlayerStateCommand::OUT_MAIN);
  mPlayerStateActionMapping["pause"] = player_onoff_action_pair_t(PlayerStateCommand::PAUSE, PlayerStateCommand::PLAY);

  //set up the double action mappings
  mPlayerDoubleActionMapping["volume"] = PlayerDoubleCommand::VOLUME;
  mPlayerDoubleActionMapping["speed"] = PlayerDoubleCommand::PLAY_SPEED;

  //set up position mappings
  mPlayerPositionActionMapping["play"] = PlayerPositionCommand::PLAY;
  mPlayerPositionActionMapping["play_relative"] = PlayerPositionCommand::PLAY_RELATIVE;
  mPlayerPositionActionMapping["loop_start"] = PlayerPositionCommand::LOOP_START;
  mPlayerPositionActionMapping["loop_end"] = PlayerPositionCommand::LOOP_END;

  for(unsigned int i = 0; i < mNumPlayers; i++) {
    dj::audio::Player * player = mMaster->players()[i];
    player->sync(true);
    player->out_state(Player::MAIN_MIX);
    player->play_state(Player::PLAY);

    //init player states
    //bool
    mPlayerStates[i]->mParamBool["mute"] = player->muted();
    mPlayerStates[i]->mParamBool["sync"] = player->syncing();
    mPlayerStates[i]->mParamBool["loop"] = player->looping();
    mPlayerStates[i]->mParamBool["cue"] = (player->out_state() == dj::audio::Player::CUE);
    mPlayerStates[i]->mParamBool["pause"] = (player->play_state() == dj::audio::Player::PAUSE);
    mPlayerStates[i]->mParamBool["audible"] = false;

    //int
    mPlayerStates[i]->mParamInt["volume"] = one_scale * player->volume();
    mPlayerStates[i]->mParamInt["speed"] = one_scale + one_scale * player->play_speed(); //percent
    mPlayerStates[i]->mParamInt["sample_rate"] = 44100;

    //position
    //XXX should we query these on each load?
    mPlayerStates[i]->mParamPosition["loop_start"] = TimePoint(-1);
    mPlayerStates[i]->mParamPosition["loop_end"] = TimePoint(-1);
  }

  //hook up and start the consume thread
  //first setup the query command + connections
  QueryPlayState * query_cmd = new QueryPlayState(mNumPlayers);
  QObject::connect(query_cmd, SIGNAL(master_value_update(QString, int)),
      SLOT(relay_master_value_changed(QString, int)),
      Qt::QueuedConnection);
  QObject::connect(query_cmd, SIGNAL(master_value_update(QString, double)),
      SLOT(relay_master_value_changed(QString, double)),
      Qt::QueuedConnection);
  QObject::connect(query_cmd, SIGNAL(master_value_update(QString, TimePoint)),
      SIGNAL(master_value_changed(QString, TimePoint)),
      Qt::QueuedConnection);
  QObject::connect(query_cmd, SIGNAL(player_value_update(int, QString, int)),
      SLOT(relay_player_value(int, QString, int)),
      Qt::QueuedConnection);
  QObject::connect(query_cmd, SIGNAL(player_bool_update(int, QString, bool)),
      SLOT(relay_player_bool(int, QString, bool)),
      Qt::QueuedConnection);

  mConsumeThread = new ConsumeThread(query_cmd, mMaster->scheduler());
  mConsumeThread->start();
}

AudioModel::~AudioModel() {
}

AudioModel * AudioModel::instance(){
  if (!cInstance)
    cInstance = new AudioModel();
  return cInstance;
}

unsigned int AudioModel::sample_rate() const { return mAudioIO->getSampleRate(); }
unsigned int AudioModel::player_count() const { return mNumPlayers; }

void AudioModel::set_player_position_frame(int player_index, int frame, bool absolute) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  if (absolute) {
    if (frame < 0)
      frame = 0;
  } else {
    //do nothing if we are doing a relative seek by 0
    if (frame == 0)
      return;
  }

  Command * cmd = NULL;
  if (absolute)
    cmd = new dj::audio::PlayerPositionCommand(
        player_index, PlayerPositionCommand::PLAY, frame);
  else
    cmd = new dj::audio::PlayerPositionCommand(
        player_index, PlayerPositionCommand::PLAY_RELATIVE, frame);
  queue_command(cmd);
}

void AudioModel::set_player_position_beat_relative(int player_index, int beats) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;
  queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_BEAT_RELATIVE, beats));
}

void AudioModel::set_player_eq(int player_index, int band, int value) {
  if (band < 0 || band > 2)
    return;

  if (value < -ione_scale)
    value = -ione_scale;
  else if (value > ione_scale)
    value = ione_scale;

  float remaped = 0.0;
  if (value > 0) {
    remaped = (6.0 * value) / (float)one_scale;
  } else {
    remaped = (70.0 * value) / (float)one_scale;
  }

  PlayerDoubleCommand::action_t action;
  QString name;
  switch(band) {
    case 0:
      action = PlayerDoubleCommand::EQ_LOW;
      name = "eq_low";
      break;
    case 1:
      action = PlayerDoubleCommand::EQ_MID;
      name = "eq_mid";
      break;
    case 2:
      action = PlayerDoubleCommand::EQ_HIGH;
      name = "eq_high";
      break;
  }

  queue_command(new PlayerDoubleCommand(player_index, action, remaped));
  emit(player_value_changed(player_index, name, value));
}

void AudioModel::relay_player_buffers_loaded(int player_index,
    AudioBufferPtr audio_buffer,
    BeatBufferPtr beat_buffer) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  PlayerState * pstate = mPlayerStates[player_index];
  if (audio_buffer)
    pstate->mNumFrames = audio_buffer->length();
  else
    pstate->mNumFrames = 0;

  //store the sample rate
  pstate->mParamInt["sample_rate"] = audio_buffer->sample_rate();

  mPlayingAudioFiles <<  audio_buffer;
  mPlayingAnnotationFiles[player_index] << beat_buffer;

  queue_command(new PlayerSetBuffersCommand(player_index, this, audio_buffer.data(), beat_buffer.data()));

  if (mCueOnLoad) {
    for (int i = 0; i < (int)mNumPlayers; i++)
      player_set(i, "cue", i == player_index);
  }

  if (beat_buffer.data() == NULL) {
    player_set(player_index, "sync", false);
    emit(player_value_changed(player_index, "update_sync_disabled", true));
  } else
    emit(player_value_changed(player_index, "update_sync_disabled", false));

  player_trigger(player_index, "reset");
  emit(player_buffers_changed(player_index, audio_buffer, beat_buffer));
  emit(player_value_changed(player_index, "audio_file", audio_buffer->file_location()));
  //XXX do beat buffer too
}

void AudioModel::relay_player_value(int player_index, QString name, int value){
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  PlayerState * pstate = mPlayerStates[player_index];

  if (name == "update_frame") {
    if ((unsigned int)value == pstate->mCurrentFrame)
      return;
    pstate->mCurrentFrame = value;
    emit(player_value_changed(player_index, name, value));
  } else if (name == "update_speed") {
    //only return rate info while syncing
    //TODO maybe send 2 values after starting to run free so that we are sure?
    if (pstate->mParamBool["sync"] || pstate->mPostFreeSpeedUpdates < 2) {
      pstate->mPostFreeSpeedUpdates += 1;
      if (pstate->mParamInt["speed"] == value)
        return;
      pstate->mParamInt["speed"] = value;
      emit(player_value_changed(player_index, name, value));
    }
  } else if (name == "update_audio_level") {
    if (value > 0)
      emit(player_value_changed(player_index, name, value));
  }
}

void AudioModel::relay_player_bool(int player_index, QString name, bool value) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  PlayerState * pstate = mPlayerStates[player_index];
  if (name == "update_audible") {
    if (pstate->mParamBool["audible"] != value) {
      pstate->mParamBool["audible"] = value;
      emit(player_value_changed(player_index, "audible", value));
      //cout << "player " << player_index << " audible: " << value << endl;
    }
  }
}

void AudioModel::relay_player_loop_frames(int player_index, long start_frame, long end_frame) {
  emit(player_value_changed(player_index, "loop_start", static_cast<int>(start_frame)));
  emit(player_value_changed(player_index, "loop_end", static_cast<int>(end_frame)));
}

void AudioModel::relay_player_looping(int player_index, bool looping) {
  emit(player_value_changed(player_index, "loop", looping));
}

void AudioModel::relay_audio_file_load_progress(int player_index, int percent){
  emit(player_value_changed(player_index, "update_progress", percent));
}

void AudioModel::relay_master_value_changed(QString name, int value) {
  emit(master_value_changed(name, value));
}

void AudioModel::relay_master_value_changed(QString name, double value) {
  if (name == "bpm")
    mMasterParamDouble["bpm"] = value;
  emit(master_value_changed(name, value));
}

void AudioModel::master_trigger(QString name) {
  if (name.contains("sync_to_player")) {
    for (int i = 0; i < static_cast<int>(mNumPlayers); i++) {
      QString idx;
      idx.setNum(i);
      if (name == (QString("sync_to_player") + idx)) {
        //emit signal before actually doing it so that the midi mapper the trigger first
        emit(master_triggered(name));
        master_set("sync_to_player", i);
        break;
      }
    }
  } else
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a master_trigger arg" << endl;
}

void AudioModel::master_set(QString name, bool value) {
  if (name == "crossfade_enabled") {
    if (value != mMasterParamBool["crossfade_enabled"]) {
      mMasterParamBool["crossfade_enabled"] = value;
      queue_command(new MasterBoolCommand(value ? MasterBoolCommand::XFADE : MasterBoolCommand::NO_XFADE));
    }
  } else
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a master_set (bool) arg" << endl;
}

void AudioModel::master_set(QString name, int value) {
  //if we have a relative value, make it absolute with the addition of the parameter in question
  make_slotarg_absolute(mMasterParamInt, name, value);
  slotval_clamp(name, value);

  if (mMasterParamInt.contains(name) && value == mMasterParamInt[name])
    return;

  if (name == "volume") {
    mMasterParamInt[name] = value;
    queue_command(new MasterDoubleCommand(MasterDoubleCommand::MAIN_VOLUME, (double)value / (double)one_scale));
    emit(master_value_changed("volume", value));
  } else if (name == "cue_volume") {
    mMasterParamInt[name] = value;
    queue_command(new MasterDoubleCommand(MasterDoubleCommand::CUE_VOLUME, (double)value / (double)one_scale));
    emit(master_value_changed("cue_volume", value));
  } else if (name == "crossfade_player_left" || name == "crossfade_player_right") {
    if (value < 0 || (unsigned int)value >= mNumPlayers) {
      cerr << DJ_FILEANDLINE << name.toStdString() << " " << value << " is out of range" << endl;
      return;
    }
    if (name.contains("left")) {
      if (mMasterParamInt["crossfade_player_left"] == value)
        return;
      mMasterParamInt["crossfade_player_left"] = value;
    } else {
      if (mMasterParamInt["crossfade_player_right"] == value)
        return;
      mMasterParamInt["crossfade_player_right"] = value;
    }
    queue_command(new MasterXFadeSelectCommand((unsigned int)mMasterParamInt["crossfade_player_left"], (unsigned int)mMasterParamInt["crossfade_player_right"]));
    emit(master_value_changed(name, value));
  } else if (name == "crossfade_position") {
    mMasterParamInt["crossfade_position"] = value;
    queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, (double)value / (double)one_scale));
    emit(master_value_changed("crossfade_position", value));
  } else if (name == "sync_to_player") {
    //return if the index is out of range or the player is already syncing
    if (value < 0 || value >= (int)mNumPlayers || mPlayerStates[value]->mParamBool["sync"])
      return;

    //create the command and connect it to our relay slot so we get a bpm update
    MasterSyncToPlayerCommand * cmd = new MasterSyncToPlayerCommand(value);
    QObject::connect(
        cmd, SIGNAL(master_value_update(QString, double)),
        SLOT(relay_master_value_changed(QString, double)),
        Qt::QueuedConnection);
    queue_command(cmd);

    if(!mPlayerStates[value]->mParamBool["sync"]) {
      mPlayerStates[value]->mParamBool["sync"] = true;
      emit(player_value_changed(value, "sync", true));
    }
  } else if (name == "track") {
    return ;//do nothing
  } else {
    cerr << name.toStdString() << " is not a master_set (int) arg" << endl;
    return;
  }
}

void AudioModel::master_set(QString name, double value) {
  //if we have a relative value, make it absolute with the addition of the parameter in question
  make_slotarg_absolute(mMasterParamDouble, name, value);
  slotval_clamp(name, value);

  if (mMasterParamDouble.contains(name) && value == mMasterParamDouble[name])
    return;

  if (name == "bpm") {
    mMasterParamDouble["bpm"] = value;
    queue_command(new TransportBPMCommand(mMaster->transport(), mMasterParamDouble["bpm"]));
    emit(master_value_changed(name, mMasterParamDouble["bpm"]));
  } else {
    cerr << DJ_FILEANDLINE << "oops, " << name.toStdString() << " not executed in audio model" << endl;
  }
}

void AudioModel::player_trigger(int player_index, QString name) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;
  PlayerState * pstate = mPlayerStates[player_index];

  if (name == "reset")
    set_player_position_frame(player_index, 0);
  else if (name == "seek_forward")
    set_player_position_beat_relative(player_index, 1);
  else if (name == "seek_back")
    set_player_position_beat_relative(player_index, -1);
  else if (name.contains("cuepoint")) {
    QString cue_index_str = name;
    cue_index_str.replace(QRegExp(".*cuepoint_"), "");
    int cue_index = cue_index_str.toInt();
    if (name.contains("set_")) {
      //XXX for now lock to closest beat
      unsigned int frame = pstate->mCurrentFrame;
      if (mPlayingAnnotationFiles[player_index].size() > 0 && mPlayingAnnotationFiles[player_index].last()) {
        //XXX ignoring file sampling rate, using global sampling rate
        BeatBufferPtr beat_buff = mPlayingAnnotationFiles[player_index].last();
        TimePoint pos = beat_buff->beat_closest(static_cast<double>(frame) / sample_rate());
        frame = static_cast<unsigned int>(beat_buff->time_at_position(pos) * static_cast<double>(sample_rate()));

        mPlayerCuepoints[player_index][cue_index] = frame;
        emit(player_value_changed(player_index, QString("set_cuepoint_%1").arg(cue_index), static_cast<int>(frame)));
      }
    } else {
      if (mPlayerCuepoints[player_index].contains(cue_index)) {
        int frame = static_cast<int>(mPlayerCuepoints[player_index][cue_index]);
        player_set(player_index, "play_frame", frame);
      }
    }
  } else if (name.contains("bump_")) {
    if(pstate->mParamBool["sync"] || pstate->mPostFreeSpeedUpdates < 2)
      return;

    double seconds = mBumpSeconds;
    if (name.contains("back")) {
      seconds = -seconds;
    }

    /*
    //scale by playback rate.. XXX should we do this?
    double speed = 1.0 + (double)pstate->mParamInt["speed"] / (double)one_scale;
    seconds *= speed;
    */

    unsigned int frames = seconds * (double)pstate->mParamInt["sample_rate"];
    queue_command(new PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, frames));
  } else if (name == "clear") {
    queue_command(new PlayerSetBuffersCommand(player_index, this, NULL, NULL));
  } else if (name == "loop") {
    player_set(player_index, "loop", true);
  } else if (name == "loop_off") {
    player_set(player_index, "loop", false);
  } else if (name == "loop_shift_back") {
    player_set(player_index, "loop_shift", -1);
  } else if (name == "loop_shift_forward") {
    player_set(player_index, "loop_shift", 1);
  } else if (name != "load") {
    PlayerState * pstate = mPlayerStates[player_index];
    QHash<QString, bool>::iterator state_itr = pstate->mParamBool.find(name);
    if (state_itr == pstate->mParamBool.end()) {
      cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_trigger arg" << endl;
      return;
    }
    //toggle
    player_set(player_index, name, !*state_itr);
  }

  emit(player_triggered(player_index, name));
}

void AudioModel::player_set(int player_index, QString name, bool value) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;
  PlayerState * pstate = mPlayerStates[player_index];

  //special case
  if (name == "seeking") {
    //pause while seeking
    if (value) {
      if (!pstate->mParamBool["pause"])
        queue_command(new dj::audio::PlayerStateCommand(player_index, PlayerStateCommand::PAUSE));
    } else {
      if (!pstate->mParamBool["pause"])
        queue_command(new dj::audio::PlayerStateCommand(player_index, PlayerStateCommand::PLAY));
    }
    return;
  } else if (name == "loop") {
    if (value) {
      PlayerLoopCommandReport * cmd = new PlayerLoopCommandReport(player_index, pstate->mLoopBeats);
      //XXX if we are currently looping, institute configurable loop behavior:
      //shrink from front vs back, grow from front vs back, 

      //hook up signal
      QObject::connect(cmd, 
          SIGNAL(player_loop_frames(int, long, long)),
          SLOT(relay_player_loop_frames(int, long, long)));

      queue_command(cmd);
    } else {
      queue_command(new PlayerStateCommand(player_index, PlayerStateCommand::NO_LOOP));
    }

    if (pstate->mParamBool["loop"] != value) {
      pstate->mParamBool["loop"] = value;
      emit(player_value_changed(player_index, "loop", value));
    }
    return;
  }

  //get the state for this name
  QHash<QString, bool>::iterator state_itr = pstate->mParamBool.find(name);
  if (state_itr == pstate->mParamBool.end()) {
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (bool) arg" << endl;
    return;
  }

  //return if there isn't anything to be done
  if (*state_itr == value)
    return;

  //get the actions
  QHash<QString, player_onoff_action_pair_t>::const_iterator action_itr = mPlayerStateActionMapping.find(name);
  if (action_itr == mPlayerStateActionMapping.end()) {
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (bool) arg [action not found]" << endl;
    return;
  }

  //set the new value
  *state_itr = value;

  //if we're going free, set our post free speed update index
  if (name == "sync" && !value)
    pstate->mPostFreeSpeedUpdates = 0;


  //queue the actual command [who's action is stored in the action_itr]
  queue_command(new dj::audio::PlayerStateCommand(player_index, value ? action_itr->first : action_itr->second));
  emit(player_value_changed(player_index, name, value));
}

void AudioModel::player_set(int player_index, QString name, int value) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;
  PlayerState * pstate = mPlayerStates[player_index];

  //make it absolute and clamp it
  make_slotarg_absolute(pstate->mParamInt, name, value);
  slotval_clamp(name, value);

  if (name == "eq_low") {
    set_player_eq(player_index, 0, value);
  } else if (name == "eq_mid") {
    set_player_eq(player_index, 1, value);
  } else if (name == "eq_high") {
    set_player_eq(player_index, 2, value);
  } else if (name == "play_frame") {
    set_player_position_frame(player_index, value, true);
  } else if (name == "play_frame_relative") {
    set_player_position_frame(player_index, value, false);
  } else if (name == "play_beat_relative") {
    set_player_position_beat_relative(player_index, value);
  } else if (name == "load") {
    emit(player_value_changed(player_index, "load", value));
  } else if (name == "loop_beats") {
    if (value > 0) {
      //unsigned int beats_last = pstate->mLoopBeats;
      pstate->mLoopBeats = player_loop_beats[value - 1];
      player_set(player_index, "loop", true);
      emit(player_value_changed(player_index, "loop_beats", value));
    } else
      player_set(player_index, "loop", false);
  } else if (name == "loop_shift") {
    PlayerLoopShiftCommandReport * cmd = new PlayerLoopShiftCommandReport(player_index, value);

    //hook up signal
    QObject::connect(cmd, 
        SIGNAL(player_loop_frames(int, long, long)),
        SLOT(relay_player_loop_frames(int, long, long)));

    queue_command(cmd);
    return;
  } else {
    //get the state for this name
    QHash<QString, int>::iterator state_itr = pstate->mParamInt.find(name);
    if (state_itr == pstate->mParamInt.end()) {
      cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (int) arg" << endl;
      return;
    }

    //return if there isn't anything to do
    if (value == *state_itr)
      return;

    //get the action
    QHash<QString, PlayerDoubleCommand::action_t>::const_iterator action_itr = mPlayerDoubleActionMapping.find(name);
    if (action_itr == mPlayerDoubleActionMapping.end()) {
      cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (int) arg [action not found]" << endl;
      return;
    }

    double dvalue = (double)value / double(one_scale);
    //speed comes in percent
    if (name == "speed") {
      //don't send speed commands if we are syncing or getting speed after going free
      if(pstate->mParamBool["sync"] || pstate->mPostFreeSpeedUpdates < 2)
        return;
      dvalue += 1;
    }

    //store value if it has changed
    if(pstate->mParamInt[name] != value)
      pstate->mParamInt[name] = value;
    else
      return;

    queue_command(new dj::audio::PlayerDoubleCommand(player_index, *action_itr, dvalue));
    emit(player_value_changed(player_index, name, value));
  }
}

void AudioModel::player_set(int player_index, QString name, double value) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;
  PlayerState * pstate = mPlayerStates[player_index];

  //make it absolute and clamp it
  make_slotarg_absolute(pstate->mParamDouble, name, value);
  slotval_clamp(name, value);

  //if (name == "play_position") {
    //if (value < 0.0)
      //value = 0.0;
    //queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY, TimePoint(value)));
  //} else if (name == "play_position_relative") {
    //queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, TimePoint(value)));
  //} else {
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (double) arg" << endl;
    return;
  //}
}

void AudioModel::player_set(int /*player_index*/, QString /*name*/, QString /*value*/) {
  //does nothing
}

void AudioModel::player_set(int player_index, QString name, dj::audio::TimePoint value) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  PlayerState * pstate = mPlayerStates[player_index];

  //get the action
  QHash<QString, PlayerPositionCommand::position_t>::const_iterator action_itr = mPlayerPositionActionMapping.find(name);
  if (action_itr == mPlayerPositionActionMapping.end()) {
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (TimePoint) arg [action not found]" << endl;
    return;
  }

  if (name.contains("loop_")) {
    if (mPlayingAnnotationFiles[player_index].size() > 0 && mPlayingAnnotationFiles[player_index].last()) {
      BeatBufferPtr beat_buff = mPlayingAnnotationFiles[player_index].last();
      //XXX ignoring file sampling rate, using global sampling rate
      int frame = beat_buff->time_at_position(value) * static_cast<double>(sample_rate());
      if (name == "loop_start" || name == "loop_end") {
        queue_command(
            new dj::audio::PlayerPositionCommand(player_index,
              name == "loop_start" ? PlayerPositionCommand::LOOP_START : PlayerPositionCommand::LOOP_END,
              frame));
        pstate->mParamPosition[name] = value;
        emit(player_value_changed(player_index, name, frame));
      }
    }
  } else {
    cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (TimePoint) arg" << endl;
    return;
  }
}

void AudioModel::player_load(int player_index, QString audio_file_path, QString annotation_file_path) {
  if (player_index < 0 || player_index >= (int)mNumPlayers)
    return;

  player_trigger(player_index, "clear");
  player_trigger(player_index, "loop_off");

  mThreadPool[player_index]->load(player_index, audio_file_path, annotation_file_path);
  //clear out the cue points
  mPlayerCuepoints[player_index].clear();
  //set the first cue point to the first frame
  mPlayerCuepoints[player_index][0] = 0;
}

void AudioModel::start_audio() {
  mAudioIO->start();

  //mAudioIO->connectToPhysical(0,4);
  //mAudioIO->connectToPhysical(1,5);
  //mAudioIO->connectToPhysical(2,2);
  //mAudioIO->connectToPhysical(3,3);

  //XXX make this configurable
  try {
    mAudioIO->connectToPhysical(0,0);
    mAudioIO->connectToPhysical(1,1);
  } catch (...) {
    cerr << "couldn't connect to physical jack audio ports" << endl;
  }
}

void AudioModel::stop_audio() {
  //there must be a better way than this!
  for(unsigned int i = 0; i < mNumPlayers; i++)
    player_trigger(i, "clear");
  usleep(500000);
  mAudioIO->stop();
  usleep(500000);
}

void AudioModel::queue_command(dj::audio::Command * cmd){
  /*
  dj::audio::CommandIOData io_data;
  cmd->store(io_data);
  cout << "queue_command " << io_data["name"] << " " << io_data["target"] << " " << io_data["value"] << endl;
  */

  mMaster->scheduler()->execute(cmd);
}

void AudioModel::slotval_clamp(const QString& param_name, int& param_value) {
  if (param_name == "volume")
    param_value = clamp(param_value, 0, one_point_five);
  else if (param_name == "speed" || param_name.contains("eq_"))
    param_value = clamp(param_value, -ione_scale, ione_scale);
  else if (param_name == "crossfade_position")
    param_value = clamp(param_value, 0, ione_scale);
  else if (param_name.contains("crossfade_player_"))
    param_value = clamp(param_value, 0, static_cast<int>(mNumPlayers));
}

void AudioModel::slotval_clamp(const QString& param_name, double& param_value) {
  if (param_name == "bpm")
    param_value = clamp(param_value, 10.0, 300.0);
}

QueryPlayState::QueryPlayState(unsigned int num_players, QObject * parent) : QObject(parent), mMasterMaxVolume(0.0) {
  mNumPlayers = num_players;
  for(unsigned int i = 0; i < mNumPlayers; i++)
    mStates.push_back(new AudioModel::PlayerState);
  mStates.resize(mNumPlayers);
}

QueryPlayState::~QueryPlayState() {
  for(unsigned int i = 0; i < mNumPlayers; i++)
    delete mStates[i];
}

bool QueryPlayState::delete_after_done() { return false; }

void QueryPlayState::execute(){
  mMasterTransportPosition = master()->transport()->position();
  mMasterMaxVolume = master()->max_sample_value();
  mMasterBPM = master()->transport()->bpm();
  master()->max_sample_value_reset();
  for(unsigned int i = 0; i < mNumPlayers; i++) {
    Player * player = master()->players()[i];
    mStates[i]->mCurrentFrame = player->frame();
    mStates[i]->mMaxSampleValue = player->max_sample_value();
    mStates[i]->mSpeed = player->play_speed();
    mStates[i]->mAudible = master()->player_audible(i);
    player->max_sample_value_reset();
  }
}

void QueryPlayState::execute_done() {
  int master_level = static_cast<int>(100.0 * mMasterMaxVolume);
  if (master_level > 0)
    emit(master_value_update("update_audio_level", master_level));
  emit(master_value_update("update_transport_position", mMasterTransportPosition));
  //emit(master_value_update("bpm", mMasterBPM));

  for(int i = 0; i < (int)mNumPlayers; i++) {
    AudioModel::PlayerState * pstate = mStates[i];
    int speed_percent = (pstate->mSpeed - 1.0) * one_scale;
    int audio_level = static_cast<int>(pstate->mMaxSampleValue * 100.0);

    emit(player_value_update(i, "update_frame", pstate->mCurrentFrame));
    emit(player_value_update(i, "update_speed", speed_percent));
    if (audio_level > 0)
      emit(player_value_update(i, "update_audio_level", audio_level));
    emit(player_bool_update(i, "update_audible", pstate->mAudible));
  }
}

//this command shouldn't be stored
bool QueryPlayState::store(CommandIOData& /* data */) const { return false; }


MasterSyncToPlayerCommand::MasterSyncToPlayerCommand(int value) :
  QObject(NULL),
  MasterIntCommand(MasterIntCommand::SYNC_TO_PLAYER, value), mBPM(0.0) {
  }

void MasterSyncToPlayerCommand::execute() {
  //execute the normal command then grab the bpm
  MasterIntCommand::execute();
  mBPM = master()->transport()->bpm();
}

void MasterSyncToPlayerCommand::execute_done() {
  emit(master_value_update("bpm", mBPM));
}

PlayerLoopCommandReport::PlayerLoopCommandReport(unsigned int idx, unsigned int beats, PlayerLoopCommand::resize_policy_t resize_policy, bool start_looping) :
  QObject(),
  PlayerLoopCommand(idx, beats, resize_policy, start_looping)
{
}

PlayerLoopCommandReport::PlayerLoopCommandReport(unsigned int idx, long start_frame, long end_frame, bool start_looping) :
  QObject(),
  PlayerLoopCommand(idx, start_frame, end_frame, start_looping)
{
}

void PlayerLoopCommandReport::execute_done() {
  emit(player_loop_frames(static_cast<int>(index()), start_frame(), end_frame()));
  emit(player_looping(static_cast<int>(index()), looping()));
}

PlayerLoopShiftCommandReport::PlayerLoopShiftCommandReport(unsigned int idx, int beats) :
  QObject(),
  PlayerLoopShiftCommand(idx, beats)
{
}

void PlayerLoopShiftCommandReport::execute_done() {
  emit(player_loop_frames(static_cast<int>(index()), start_frame(), end_frame()));
  emit(player_looping(static_cast<int>(index()), looping()));
}
