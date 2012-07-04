#include "audiomodel.hpp"
#include "loaderthread.hpp"
#include "transport.hpp"
#include "defines.hpp"

#include <vector>
#include <QMetaObject>
#include <QTimer>

using namespace dj;
using namespace dj::audio;

#include <iostream>
using std::cerr;
using std::endl;

namespace {
   const int audible_timeout_ms = 200;

   void init_signal_hashes() {
      QStringList list;
      
      //*** PLAYER

      list << "load" << "reset" << "seek_forward" << "seek_back" << "bump_forward" << "bump_back";
      AudioModel::player_signals["trigger"] = list;

      list.clear();
      list << "cue" << "sync" << "pause";
      AudioModel::player_signals["bool"] = list;

      list.clear();
      list << "volume" << "speed" << "eq_low" << "eq_mid" << "eq_high" << "play_frame" << "play_beat";
      AudioModel::player_signals["int"] = list;

      list.clear();
      list << "play_position";
      AudioModel::player_signals["double"] = list;

      //*** MASTER

      list.clear();
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

template <typename T>
T clamp(T val, T bottom, T top) {
   if (val < bottom)
      return bottom;
   if (val > top)
      return top;
   return val;
}


QHash<QString, QStringList> AudioModel::player_signals;
QHash<QString, QStringList> AudioModel::master_signals;

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
            mAudioModel->mPlayingAnnotationFiles.removeOne(buffer);
         }
         //execute the super class's done action
         PlayerCommand::execute_done();
      }
      virtual bool store(CommandIOData& /*data*/) const {
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
         mMaxSampleValue(0.0) { }

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
   mBumpSeconds(0.25)
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
   mPlayerStateActionMapping["loop"] = player_onoff_action_pair_t(PlayerStateCommand::LOOP, PlayerStateCommand::NO_LOOP);
   mPlayerStateActionMapping["cue"] = player_onoff_action_pair_t(PlayerStateCommand::OUT_CUE, PlayerStateCommand::OUT_MAIN);
   mPlayerStateActionMapping["pause"] = player_onoff_action_pair_t(PlayerStateCommand::PAUSE, PlayerStateCommand::PLAY);

   //set up the double action mappings
   mPlayerDoubleActionMapping["volume"] = PlayerDoubleCommand::VOLUME;
   mPlayerDoubleActionMapping["speed"] = PlayerDoubleCommand::PLAY_SPEED;

   //set up position mappings
   mPlayerPositionActionMapping["play"] = PlayerPositionCommand::PLAY;
   mPlayerPositionActionMapping["play_relative"] = PlayerPositionCommand::PLAY_RELATIVE;
   mPlayerPositionActionMapping["start"] = PlayerPositionCommand::START;
   mPlayerPositionActionMapping["end"] = PlayerPositionCommand::END;
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
      mPlayerStates[i]->mParamPosition["start"] = player->start_position();
      //XXX should we query these on each load?
      mPlayerStates[i]->mParamPosition["end"] = TimePoint(-1);
      mPlayerStates[i]->mParamPosition["loop_start"] = TimePoint(-1);
      mPlayerStates[i]->mParamPosition["loop_end"] = TimePoint(-1);
   }

   //hook up and start the consume thread
   //first setup the query command + connections
   QueryPlayState * query_cmd = new QueryPlayState(mNumPlayers);
   QObject::connect(query_cmd, SIGNAL(master_value_update(QString, int)),
            SIGNAL(master_value_changed(QString, int)),
            Qt::QueuedConnection);
   QObject::connect(query_cmd, SIGNAL(master_value_update(QString, TimePoint)),
            SIGNAL(master_value_changed(QString, TimePoint)),
            Qt::QueuedConnection);
   QObject::connect(query_cmd, SIGNAL(player_value_update(int, QString, int)),
            SLOT(relay_player_value(int, QString, int)),
            Qt::QueuedConnection);

   mConsumeThread = new ConsumeThread(query_cmd, mMaster->scheduler());
   mConsumeThread->start();

   mAudibleTimer = new QTimer(this);
   mAudibleTimer->setInterval(audible_timeout_ms);
   QObject::connect(mAudibleTimer, SIGNAL(timeout()), SLOT(players_eval_audible()));
   mAudibleTimer->start();
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

void AudioModel::set_player_position(int player_index, const TimePoint &val, bool absolute){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (absolute)
      cmd = new dj::audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, val);
   else
      cmd = new dj::audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY_RELATIVE, val);
   queue_command(cmd);
}

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
   TimePoint point;
   unsigned int abs_beat = abs(beats);
   int bar = 0;
   while(abs_beat >= 4) {
      abs_beat -= 4;
      bar += 1;
   }
   point.at_bar(bar, abs_beat);
   if (beats < 0)
      point = -point;

   set_player_position(player_index, point, false);
}

void AudioModel::set_player_eq(int player_index, int band, int value) {
   if (band < 0 || band > 2)
      return;

   int ione_scale = one_scale;

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
   mPlayingAnnotationFiles << beat_buffer;

   queue_command(new PlayerSetBuffersCommand(player_index, this, audio_buffer.data(), beat_buffer.data()));

   if (beat_buffer.data() == NULL) {
      player_set(player_index, "sync", false);
      emit(player_value_changed(player_index, "update_sync_disabled", true));
   } else
      emit(player_value_changed(player_index, "update_sync_disabled", false));

   player_trigger(player_index, "reset");
   emit(player_buffers_changed(player_index, audio_buffer, beat_buffer));
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

void AudioModel::relay_audio_file_load_progress(int player_index, int percent){
   emit(player_value_changed(player_index, "update_progress", percent));
}

void AudioModel::players_eval_audible() {
   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++)
      player_eval_audible(player_index);
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
   
   if (name == "volume") {
      value = clamp(value, 0, (int)(1.5 * one_scale));
      if (value == mMasterParamInt[name])
         return;
      mMasterParamInt[name] = value;
      queue_command(new MasterDoubleCommand(MasterDoubleCommand::MAIN_VOLUME, (double)value / (double)one_scale));
      emit(master_value_changed("volume", value));
   } else if (name == "cue_volume") {
      value = clamp(value, 0, (int)(1.5 * one_scale));
      if (value == mMasterParamInt[name])
         return;
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
      value = clamp(value, 0, (int)one_scale);
      if (value == mMasterParamInt[name])
         return;
      mMasterParamInt["crossfade_position"] = value;
      queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, (double)value / (double)one_scale));
      emit(master_value_changed("crossfade_position", value));
   } else if (name == "sync_to_player") {
      if (value < 0 || value >= (int)mNumPlayers)
         return;

      queue_command(new MasterIntCommand(MasterIntCommand::SYNC_TO_PLAYER, value));

      if(!mPlayerStates[value]->mParamBool["sync"]) {
         mPlayerStates[value]->mParamBool["sync"] = true;
         emit(player_value_changed(value, "sync", true));
      }
   } else if (name == "track") {
      //do nothing
   } else
      cerr << name.toStdString() << " is not a master_set (int) arg" << endl;
}

void AudioModel::master_set(QString name, double value) {
   //if we have a relative value, make it absolute with the addition of the parameter in question
   make_slotarg_absolute(mMasterParamDouble, name, value);

   if (name == "bpm") {
      if (value != mMasterParamDouble["bpm"]) {
         mMasterParamDouble["bpm"] = value;
         queue_command(new TransportBPMCommand(mMaster->transport(), mMasterParamDouble["bpm"]));
         emit(master_value_changed(name, mMasterParamDouble["bpm"]));
      }
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
   else if (name.contains("bump_")) {
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
   } else if (name == "clear")
      queue_command(new PlayerSetBuffersCommand(player_index, this, NULL, NULL));
   else if (name != "load") {
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

   //if we'return going free, set our post free speed update index
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

   //make it absolute
   make_slotarg_absolute(pstate->mParamInt, name, value);

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

   //make it absolute
   make_slotarg_absolute(pstate->mParamDouble, name, value);

   if (name == "play_position") {
      if (value < 0.0)
         value = 0.0;
      queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY, TimePoint(value)));
   } else if (name == "play_position_relative") {
      queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, TimePoint(value)));
   } else {
      cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (double) arg" << endl;
      return;
   }
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
   
   if (name == "play") {
      queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY, value));
   } else if (name == "play_relative") {
      queue_command(new dj::audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, value));
   } else {
      //get the state for this name
      QHash<QString, TimePoint>::iterator state_itr = pstate->mParamPosition.find(name);
      if (state_itr == pstate->mParamPosition.end()) {
         cerr << DJ_FILEANDLINE << name.toStdString() << " is not a valid player_set (TimePoint) arg" << endl;
         return;
      }
      //return if there isn't anything to be done
      if (*state_itr == value)
         return;

      //set the new value
      *state_itr = value;

      //queue the actual command [who's action is stored in the action_itr]
      queue_command(new dj::audio::PlayerPositionCommand(player_index, *action_itr, value));

      //XXX TODO emit(player_position_changed(player_index, name, value));
   }
}

void AudioModel::player_load(int player_index, QString audio_file_path, QString annotation_file_path) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   player_trigger(player_index, "clear");
   mThreadPool[player_index]->load(player_index, audio_file_path, annotation_file_path);
}

void AudioModel::start_audio() {
   mAudioIO->start();

   //mAudioIO->connectToPhysical(0,4);
   //mAudioIO->connectToPhysical(1,5);
   //mAudioIO->connectToPhysical(2,2);
   //mAudioIO->connectToPhysical(3,3);

   //XXX make this configurable
   mAudioIO->connectToPhysical(0,0);
   mAudioIO->connectToPhysical(1,1);
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
   mMaster->scheduler()->execute(cmd);
}

void AudioModel::player_eval_audible(int player_index) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   PlayerState * state = mPlayerStates[player_index];
   bool audible = true;

   if(state->mNumFrames == 0 ||
         state->mCurrentFrame >= state->mNumFrames ||
         state->mParamBool["mute"] ||
         state->mParamBool["pause"] ||
         //XXX detect player cue style: player->mParamBool["cue"]
         state->mParamInt["volume"] < mPlayerAudibleThresholdVolume ||
         (mMasterParamBool["crossfade_enabled"] && 
          ((mMasterParamInt["crossfade_player_right"] == player_index && mMasterParamInt["crossfade_position"] < mCrossfadeAudibleThresholdPosition) ||
          (mMasterParamInt["crossfade_player_left"] == player_index && mMasterParamInt["crossfade_position"] > (one_scale - mCrossfadeAudibleThresholdPosition))))
         ) {
      audible = false;
   }

   if (state->mParamBool["audible"] != audible) {
      state->mParamBool["audible"] = audible;
      emit(player_value_changed(player_index, "audible", audible));
   } 
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
   master()->max_sample_value_reset();
   for(unsigned int i = 0; i < mNumPlayers; i++) {
      Player * player = master()->players()[i];
      mStates[i]->mCurrentFrame = player->frame();
      mStates[i]->mMaxSampleValue = player->max_sample_value();
      mStates[i]->mSpeed = player->play_speed();
      player->max_sample_value_reset();
   }
}

void QueryPlayState::execute_done() {
   int master_level = static_cast<int>(100.0 * mMasterMaxVolume);
   if (master_level > 0)
      emit(master_value_update("update_audio_level", master_level));
   emit(master_value_update("update_transport_position", mMasterTransportPosition));

   for(int i = 0; i < (int)mNumPlayers; i++) {
      AudioModel::PlayerState * pstate = mStates[i];
      int speed_percent = (pstate->mSpeed - 1.0) * one_scale;
      int audio_level = static_cast<int>(pstate->mMaxSampleValue * 100.0);

      emit(player_value_update(i, "update_frame", pstate->mCurrentFrame));
      emit(player_value_update(i, "update_speed", speed_percent));
      if (audio_level > 0)
         emit(player_value_update(i, "update_audio_level", audio_level));
   }
}

//this command shouldn't be stored
bool QueryPlayState::store(CommandIOData& /* data */) const { return false; }

