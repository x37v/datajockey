#include "audiomodel.hpp"
#include "audioloaderthread.hpp"
#include "audiobufferreference.hpp"
#include "transport.hpp"
#include "defines.hpp"

#include <QMutexLocker>
#include <vector>
#include <QMetaObject>

using namespace DataJockey;
using namespace DataJockey::Audio;

#include <iostream>
using std::cerr;
using std::endl;

template <typename T>
T clamp(T val, T bottom, T top) {
   if (val < bottom)
      return bottom;
   if (val > top)
      return top;
   return val;
}

class AudioModel::PlayerClearBuffersCommand : public DataJockey::Audio::PlayerCommand {
   public:
      PlayerClearBuffersCommand(unsigned int idx,
            AudioModel * model,
            QString oldFileName = QString()) :
         DataJockey::Audio::PlayerCommand(idx),
         mAudioModel(model),
         mOldFileName(oldFileName),
         mOldBeatBuffer(NULL) { }
      virtual ~PlayerClearBuffersCommand() { }
      virtual void execute() {
         Player * p = player(); 
         if(p != NULL){
            mOldBeatBuffer = p->beat_buffer();
            p->audio_buffer(NULL);
            p->beat_buffer(NULL);
         }
      }
      virtual void execute_done() {
         if (!mOldFileName.isEmpty()) {
            //low level, decrement reference
            AudioBufferReference::decrement_count(mOldFileName);
         }
         //delete the old beat
         if (mOldBeatBuffer != NULL) {
            delete mOldBeatBuffer;
            mOldBeatBuffer = NULL;
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
      QString mOldFileName;
      BeatBuffer * mOldBeatBuffer;
};

class AudioModel::PlayerState {
   public:
      PlayerState() :
         mFileName(),
         mBeatBuffer(),
         mInBeatBufferTransaction(false),
         mCurrentFrame(0) { }
      //not okay to update in audio thread
      QString mFileName;
      Audio::BeatBuffer mBeatBuffer;
      bool mInBeatBufferTransaction;

      QMap<QString, int> mParamInt;
      QMap<QString, bool> mParamBool;
      QMap<QString, TimePoint> mParamPosition;


      /*
      unsigned int mVolume;
      unsigned int mPlaySpeed;
      bool mMute;
      bool mSync;
      bool mLoop;
      bool mCue;
      bool mPause;
      */

      //okay to update in audio thread
      unsigned int mCurrentFrame;
};

//TODO how to get it to run at the end of the frame?
class AudioModel::QueryPlayerStates : public MasterCommand {
   private:
      AudioModel * mAudioModel;
   public:
      std::vector<AudioModel::PlayerState* > mStates;
      unsigned int mNumPlayers;

      QueryPlayerStates(AudioModel * model) : mAudioModel(model) {
         mNumPlayers = mAudioModel->player_count();
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mStates.push_back(new AudioModel::PlayerState);
         mStates.resize(mNumPlayers);
      }
      virtual ~QueryPlayerStates() {
         for(unsigned int i = 0; i < mNumPlayers; i++)
            delete mStates[i];
      }
      virtual void execute(){
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mStates[i]->mCurrentFrame = master()->players()[i]->frame();
      }
      virtual void execute_done() {
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mAudioModel->update_player_state(i, mStates[i]);
      }
      //this command shouldn't be stored
      virtual bool store(CommandIOData& /* data */) const { return false; }
};

class AudioModel::ConsumeThread : public QThread {
   private:
      Scheduler * mScheduler;
      AudioModel * mModel;
   public:
      ConsumeThread(AudioModel * model, Scheduler * scheduler) : mScheduler(scheduler), mModel(model) { }

      void run() {
         while(true) {
            AudioModel::QueryPlayerStates * cmd = new AudioModel::QueryPlayerStates(mModel);
            mScheduler->execute(cmd);
            mScheduler->execute_done_actions();
            msleep(15);
         }
      }
};

AudioModel * AudioModel::cInstance = NULL;

AudioModel::AudioModel() :
   QObject(),
   mPlayerStates(),
   mPlayerStatesMutex(QMutex::Recursive),
   mMasterBPM(0.0)
{
   unsigned int num_players = 2;

   mAudioIO = DataJockey::Audio::AudioIO::instance();
   mMaster = DataJockey::Audio::Master::instance();

   mNumPlayers = num_players;
   for(unsigned int i = 0; i < mNumPlayers; i++) {
      mMaster->add_player();
      mPlayerStates.push_back(new PlayerState());
      AudioLoaderThread * newThread = new AudioLoaderThread(this);
      mThreadPool.push_back(newThread);
      QObject::connect(newThread, SIGNAL(load_progress(QString, int)),
            this, SLOT(relay_audio_file_load_progress(QString, int)),
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
      DataJockey::Audio::Player * player = mMaster->players()[i];
      player->sync(true);
      player->out_state(Player::MAIN_MIX);
      player->play_state(Player::PLAY);

      //init player states
      //bool
      mPlayerStates[i]->mParamBool["mute"] = player->muted();
      mPlayerStates[i]->mParamBool["sync"] = player->syncing();
      mPlayerStates[i]->mParamBool["loop"] = player->looping();
      mPlayerStates[i]->mParamBool["cue"] = (player->out_state() == DataJockey::Audio::Player::CUE);
      mPlayerStates[i]->mParamBool["pause"] = (player->play_state() == DataJockey::Audio::Player::PAUSE);

      //int
      mPlayerStates[i]->mParamInt["volume"] = one_scale * player->volume();
      mPlayerStates[i]->mParamInt["speed"] = one_scale * player->play_speed();

      //position
      mPlayerStates[i]->mParamPosition["start"] = player->start_position();
      //XXX should we query these on each load?
      mPlayerStates[i]->mParamPosition["end"] = TimePoint(-1);
      mPlayerStates[i]->mParamPosition["loop_start"] = TimePoint(-1);
      mPlayerStates[i]->mParamPosition["loop_end"] = TimePoint(-1);
   }

   //hook up and start the consume thread
   mConsumeThread = new ConsumeThread(this, mMaster->scheduler());
   mConsumeThread->start();

   //internal signal connections
   /*
   QObject::connect(this, SIGNAL(relay_player_audio_file_changed(int, QString)),
         this, SIGNAL(player_audio_file_changed(int, QString)),
         Qt::QueuedConnection);

   QObject::connect(this, SIGNAL(relay_player_position_changed(int, int)),
         this, SIGNAL(player_position_changed(int, int)),
         Qt::QueuedConnection);
         */
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

QString AudioModel::player_audio_file(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return QString();
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mFileName;
}

BeatBuffer AudioModel::player_beat_buffer(int player_index) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return BeatBuffer();
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mBeatBuffer;
}

double AudioModel::master_bpm() const {
   return mMasterBPM;
}


void AudioModel::set_player_position(int player_index, const TimePoint &val, bool absolute){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (absolute)
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, val);
   else
      cmd = new DataJockey::Audio::PlayerPositionCommand(
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
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, frame);
   else
      cmd = new DataJockey::Audio::PlayerPositionCommand(
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

void AudioModel::set_player_audio_buffer(int player_index, AudioBuffer * buf){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new PlayerSetAudioBufferCommand(player_index, buf));
}

void AudioModel::set_player_beat_buffer(int player_index, QString buffer_file) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //update our state
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];
   player_state->mInBeatBufferTransaction = false;

   //XXX what if we fail to load?
   //defer this to a thread?
   if (buffer_file.isEmpty())
      player_state->mBeatBuffer.clear();
   else
      player_state->mBeatBuffer.load(buffer_file.toStdString());
   BeatBuffer * player_buf = new DataJockey::Audio::BeatBuffer(player_state->mBeatBuffer);
   queue_command(new PlayerSetBeatBufferCommand(player_index, player_buf, true));

   emit(player_beat_buffer_changed(player_index));
}

void AudioModel::set_player_buffers(int player_index, QString audio_file, QString beat_file) {
   set_player_audio_file(player_index, audio_file);
   set_player_beat_buffer(player_index, beat_file);
}

void AudioModel::update_player_state(int player_index, PlayerState * state){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   int frame = state->mCurrentFrame;
   if ((unsigned int)frame != mPlayerStates[player_index]->mCurrentFrame) {
      mPlayerStates[player_index]->mCurrentFrame = frame;
      //emit(relay_player_position_changed(player_index, frame));
      QMetaObject::invokeMethod(this, "relay_player_position_changed", 
            Qt::QueuedConnection,
            Q_ARG(int, player_index),
            Q_ARG(int, frame));
   }
}

void AudioModel::set_player_audio_file(int player_index, QString location){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);

   //only update if new
   QString oldfile = mPlayerStates[player_index]->mFileName;
   if (oldfile != location) {
      //use the low level method because we're passing through the lock free buffer
      AudioBuffer * buf = AudioBufferReference::get_and_increment_count(location);

      //clear out the old buffers
      set_player_clear_buffers(player_index);

      //once the manager contains the location we know that it is full loaded
      //but, if not it could actually be in progress
      if (buf == NULL) {
         bool loading = false;

         //see if a thread is already loading it
         for(unsigned int i = 0; i < mThreadPool.size(); i++) {
            if (mThreadPool[i]->file_name() == location) {
               loading = true;
               break;
            }
         }

         //if the file isn't already loading then start up our thread and load it
         if (!loading) {
            if (mThreadPool[player_index]->isRunning()) {
               mThreadPool[player_index]->abort();
               mThreadPool[player_index]->wait();
            }
            try {
               mThreadPool[player_index]->load(location);
            } catch(...) {
               //TODO report error
            }
         }

         //update player state
         mPlayerStates[player_index]->mFileName = location;

      } else {
         //send the stored buf
         set_player_audio_buffer(player_index, buf);

         //update player state
         mPlayerStates[player_index]->mFileName = location;

         //notify
         emit(player_audio_file_changed(player_index, location));
      }
   }
}

void AudioModel::set_player_clear_buffers(int player_index) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);

   QString oldFileName = mPlayerStates[player_index]->mFileName;
   if (!oldFileName.isEmpty()) {
      //this command will clear out the buffers and decrement the reference to the audio buffer
      queue_command(new PlayerClearBuffersCommand(player_index, this, oldFileName));
      mPlayerStates[player_index]->mFileName.clear();
      //notify
      emit(player_audio_file_cleared(player_index));
   }
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

void AudioModel::relay_player_audio_file_changed(int player_index, QString fileName){
   emit(player_audio_file_changed(player_index, fileName));
}

void AudioModel::relay_player_position_changed(int player_index, int frame_index){
   emit(player_value_changed(player_index, "frame", frame_index));
}

void AudioModel::relay_audio_file_load_progress(QString fileName, int percent){
   QMutexLocker lock(&mPlayerStatesMutex);
   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++) {
      if (mPlayerStates[player_index]->mFileName == fileName)
         emit(player_value_changed(player_index, "progress", percent));
   }
}

//called from another thread
bool AudioModel::audio_file_load_complete(QString fileName, AudioBuffer * buffer){
   QMutexLocker lock(&mPlayerStatesMutex);

   if (!buffer || !buffer->loaded())
      return false;

   //we want to see if any player loaded it
   bool loaded_into_a_player = false;

   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++) {
      //operate only on the players which are loading this file
      if (mPlayerStates[player_index]->mFileName != fileName)
         continue;

      //update the manager
      AudioBufferReference::set_or_increment_count(fileName, buffer);
      loaded_into_a_player = true;

      set_player_audio_buffer(player_index, buffer);

      //reset player position
      set_player_position(player_index, TimePoint(0.0));

      //calling from another thread, emit a signal which will then be passed along
      //emit(relay_player_audio_file_changed(player_index, fileName));
      QMetaObject::invokeMethod(this, "relay_player_audio_file_changed", 
            Qt::QueuedConnection,
            Q_ARG(int, player_index),
            Q_ARG(QString, fileName));
   }

   if (!loaded_into_a_player)
      return false;
   return true;
}

void AudioModel::set_player_beat_buffer_clear(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];

   //clear out our local beat buffer and send an empty one to the audio thread
   //also, indicate that we want the old buffer to be deallocated once it is cleared
   player_state->mBeatBuffer.clear();
   if(!player_state->mInBeatBufferTransaction)
      queue_command(new PlayerSetBeatBufferCommand(player_index, NULL, true));
}

void AudioModel::set_player_beat_buffer_begin(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];
   player_state->mInBeatBufferTransaction = true;

}

void AudioModel::set_player_beat_buffer_end(int player_index, bool commit){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);

   //update our state
   PlayerState * player_state = mPlayerStates[player_index];
   player_state->mInBeatBufferTransaction = false;

   //if this is a commit then send the buffer
   if (commit) {
      BeatBuffer * buff = new DataJockey::Audio::BeatBuffer(player_state->mBeatBuffer);
      //deallocate old buffer
      queue_command(new PlayerSetBeatBufferCommand(player_index, buff, true));
   }
}

void AudioModel::set_player_beat_buffer_add_beat(int player_index, double value){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];

   //insert the beat
   player_state->mBeatBuffer.insert_beat(value);

   //if we're not in a transaction then send the new beat buffer
   if(!player_state->mInBeatBufferTransaction) {
      BeatBuffer * buff = new DataJockey::Audio::BeatBuffer(player_state->mBeatBuffer);
      //deallocate old buffer
      queue_command(new PlayerSetBeatBufferCommand(player_index, buff, true));
   }
}

void AudioModel::set_player_beat_buffer_remove_beat(int /*player_index*/, double /*value*/){
   /*
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];
   */

   //TODO
}

void AudioModel::set_player_beat_buffer_update_beat(int player_index, int beat_index, double new_value){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * player_state = mPlayerStates[player_index];

   //update the beat
   player_state->mBeatBuffer.update_value(beat_index, new_value);

   //if we're not in a transaction then send the new beat buffer
   if(!player_state->mInBeatBufferTransaction) {
      BeatBuffer * buff = new DataJockey::Audio::BeatBuffer(player_state->mBeatBuffer);
      queue_command(new PlayerSetBeatBufferCommand(player_index, buff, true));
   }
}

void AudioModel::master_set(QString name, bool value) {

   if (name == "crossfade") {
      queue_command(new MasterBoolCommand(value ? MasterBoolCommand::XFADE : MasterBoolCommand::NO_XFADE));
   } else
      cerr << name.toStdString() << " is not a master_set (bool) arg" << endl;
}

void AudioModel::master_set(QString name, int value) {
   //TODO compare against last set value?
   
   if (name == "volume") {
      value = clamp(value, 0, (int)(1.5 * one_scale));
      queue_command(new MasterDoubleCommand(MasterDoubleCommand::MAIN_VOLUME, (double)value / (double)one_scale));
      emit(master_value_changed("volume", value));
   } else if (name == "cue_volume") {
      value = clamp(value, 0, (int)(1.5 * one_scale));
      queue_command(new MasterDoubleCommand(MasterDoubleCommand::CUE_VOLUME, (double)value / (double)one_scale));
      emit(master_value_changed("cue_volume", value));
   } else if (name == "crossfade_position") {
      value = clamp(value, 0, (int)one_scale);
      queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, (double)value / (double)one_scale));
      emit(master_value_changed("crossfade_position", value));
   } else if (name == "sync_to_player") {
      if (value < 0 || value >= (int)mNumPlayers)
         return;

      QMutexLocker lock(&mPlayerStatesMutex);
      queue_command(new MasterIntCommand(MasterIntCommand::SYNC_TO_PLAYER, value));

      if(!mPlayerStates[value]->mParamBool["sync"]) {
         mPlayerStates[value]->mParamBool["sync"] = true;
         emit(player_toggled(value, "sync", true));
      }
   } else
      cerr << name.toStdString() << " is not a master_set (int) arg" << endl;
}

void AudioModel::set_master_cross_fade_players(int left, int right){
   if (left < 0 || left >= (int)mNumPlayers)
      return;
   if (right < 0 || right >= (int)mNumPlayers)
      return;
   queue_command(new MasterXFadeSelectCommand((unsigned int)left, (unsigned int)right));
}

void AudioModel::set_master_bpm(double bpm) {
   if (bpm != mMasterBPM) {
      mMasterBPM = bpm;
      queue_command(new TransportBPMCommand(mMaster->transport(), bpm));
      emit(master_bpm_changed(bpm));
   }
}

bool AudioModel::player_state_bool(int player_index, QString name) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * pstate = mPlayerStates[player_index];

   QMap<QString, bool>::const_iterator itr = pstate->mParamBool.find(name);
   if (itr == pstate->mParamBool.end())
      return false;
   return *itr;
}

int AudioModel::player_state_int(int player_index, QString name) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * pstate = mPlayerStates[player_index];

   if (name == "frame") {
      return pstate->mCurrentFrame;
   }

   QMap<QString, int>::const_iterator itr = pstate->mParamInt.find(name);
   if (itr == pstate->mParamInt.end())
      return 0;
   return *itr;
}

void AudioModel::player_trigger(int player_index, QString name) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   if (name == "reset")
      set_player_position_frame(player_index, 0);
   else if (name == "seek_forward")
      set_player_position_beat_relative(player_index, 1);
   else if (name == "seek_back")
      set_player_position_beat_relative(player_index, -1);
   else if (name != "load")
      cerr << name.toStdString() << " is not a valid player_trigger arg" << endl;
}

void AudioModel::player_set(int player_index, QString name, bool value) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * pstate = mPlayerStates[player_index];

   //special case
   if (name == "seeking") {
      //pause while seeking
      if (value) {
         if (!pstate->mParamBool["pause"])
            queue_command(new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::PAUSE));
      } else {
         if (!pstate->mParamBool["pause"])
            queue_command(new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::PLAY));
      }
      return;
   }

   //get the state for this name
   QMap<QString, bool>::iterator state_itr = pstate->mParamBool.find(name);
   if (state_itr == pstate->mParamBool.end()) {
      cerr << name.toStdString() << " is not a valid player_set (bool) arg" << endl;
      return;
   }

   //return if there isn't anything to be done
   if (*state_itr == value)
      return;

   //get the actions
   QMap<QString, player_onoff_action_pair_t>::const_iterator action_itr = mPlayerStateActionMapping.find(name);
   if (action_itr == mPlayerStateActionMapping.end()) {
      cerr << name.toStdString() << " is not a valid player_set (bool) arg [action not found]" << endl;
      return;
   }

   //set the new value
   *state_itr = value;

   //queue the actual command [who's action is stored in the action_itr]
   queue_command(new DataJockey::Audio::PlayerStateCommand(player_index, value ? action_itr->first : action_itr->second));
   emit(player_toggled(player_index, name, value));
}

void AudioModel::player_set(int player_index, QString name, int value) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * pstate = mPlayerStates[player_index];

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
      QMap<QString, int>::iterator state_itr = pstate->mParamInt.find(name);
      if (state_itr == pstate->mParamInt.end()) {
         cerr << name.toStdString() << " is not a valid player_set (int) arg" << endl;
         return;
      }

      //return if there isn't anything to do
      if (value == *state_itr)
         return;

      //get the action
      QMap<QString, PlayerDoubleCommand::action_t>::const_iterator action_itr = mPlayerDoubleActionMapping.find(name);
      if (action_itr == mPlayerDoubleActionMapping.end()) {
         cerr << name.toStdString() << " is not a valid player_set (int) arg [action not found]" << endl;
         return;
      }

      queue_command(new DataJockey::Audio::PlayerDoubleCommand(player_index, *action_itr, (double)value / double(one_scale)));
   }
}

void AudioModel::player_set(int player_index, QString name, double value) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   if (name == "play_position") {
      if (value < 0.0)
         value = 0.0;
      queue_command(new DataJockey::Audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY, TimePoint(value)));
   } else if (name == "play_position_relative") {
      queue_command(new DataJockey::Audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, TimePoint(value)));
   } else {
      cerr << name.toStdString() << " is not a valid player_set (double) arg" << endl;
      return;
   }
}

void AudioModel::player_set(int player_index, QString name, DataJockey::Audio::TimePoint value) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   PlayerState * pstate = mPlayerStates[player_index];

   //get the action
   QMap<QString, PlayerPositionCommand::position_t>::const_iterator action_itr = mPlayerPositionActionMapping.find(name);
   if (action_itr == mPlayerPositionActionMapping.end()) {
      cerr << name.toStdString() << " is not a valid player_set (TimePoint) arg [action not found]" << endl;
      return;
   }
   
   if (name == "play") {
      queue_command(new DataJockey::Audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY, value));
   } else if (name == "play_relative") {
      queue_command(new DataJockey::Audio::PlayerPositionCommand(player_index, PlayerPositionCommand::PLAY_RELATIVE, value));
   } else {
      //get the state for this name
      QMap<QString, TimePoint>::iterator state_itr = pstate->mParamPosition.find(name);
      if (state_itr == pstate->mParamPosition.end()) {
         cerr << name.toStdString() << " is not a valid player_set (TimePoint) arg" << endl;
         return;
      }
      //return if there isn't anything to be done
      if (*state_itr == value)
         return;

      //set the new value
      *state_itr = value;

      //queue the actual command [who's action is stored in the action_itr]
      queue_command(new DataJockey::Audio::PlayerPositionCommand(player_index, *action_itr, value));

      //XXX TODO emit(player_position_changed(player_index, name, value));
   }


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
      set_player_clear_buffers(i);
   usleep(500000);
   mAudioIO->stop();
   usleep(500000);
}

void AudioModel::queue_command(DataJockey::Audio::Command * cmd){
   mMaster->scheduler()->execute(cmd);
}

