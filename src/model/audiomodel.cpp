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
using std::cout;
using std::endl;

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

      //okay to update in audio thread
      unsigned int mVolume;
      unsigned int mPlaySpeed;
      bool mMute;
      bool mSync;
      bool mLoop;
      bool mCue;
      bool mPause;
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
            msleep(10);
         }
      }
};

AudioModel * AudioModel::cInstance = NULL;

AudioModel::AudioModel() :
   QObject(),
   mPlayerStates(),
   mPlayerStatesMutex(QMutex::Recursive)
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

   for(unsigned int i = 0; i < mNumPlayers; i++) {
      DataJockey::Audio::Player * player = mMaster->players()[i];
      player->sync(true);
      player->out_state(Player::MAIN_MIX);
      player->play_state(Player::PLAY);

      //init player states
      //bool
      mPlayerStates[i]->mMute = player->muted();
      mPlayerStates[i]->mSync = player->syncing();
      mPlayerStates[i]->mLoop = player->looping();
      mPlayerStates[i]->mCue = (player->out_state() == DataJockey::Audio::Player::CUE);
      mPlayerStates[i]->mPause = (player->play_state() == DataJockey::Audio::Player::PAUSE);

      //int
      mPlayerStates[i]->mVolume = one_scale * player->volume();
      mPlayerStates[i]->mPlaySpeed = one_scale * player->play_speed();
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

//*************** getters

unsigned int AudioModel::player_count() const { return mNumPlayers; }

bool AudioModel::player_pause(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mPause;
}

bool AudioModel::player_cue(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mCue;
}

//void AudioModel::player_out_state(int player_index, Audio::Player::out_state_t val){ } 
//void AudioModel::player_stretch_method(int player_index, Audio::Player::stretch_method_t val){ }

bool AudioModel::player_mute(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mMute;
}

bool AudioModel::player_sync(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mSync;
}

bool AudioModel::player_loop(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mLoop;
}

int AudioModel::player_volume(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mVolume;
}

int AudioModel::player_play_speed(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mPlaySpeed;
}

//void AudioModel::player_position(int player_index){ }
//void AudioModel::player_start_position(int player_index){ }
//void AudioModel::player_end_position(int player_index){ }
//void AudioModel::player_loop_start_position(int player_index){ }
//void AudioModel::player_loop_end_position(int player_index){ }

QString AudioModel::player_audio_file(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return QString();
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mFileName;
}

//****************** setters/slots

void AudioModel::set_player_pause(int player_index, bool pause){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mPause != pause) {
      Command * cmd = NULL;
      if (pause)
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::PAUSE);
      else
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::PLAY);
      queue_command(cmd);
      mPlayerStates[player_index]->mPause = pause;
      emit(player_pause_changed(player_index, pause));
   }
}

void AudioModel::set_player_cue(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mCue != val) {
      Command * cmd = NULL;
      if (val)
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::OUT_CUE);
      else
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, PlayerStateCommand::OUT_MAIN);
      queue_command(cmd);
      mPlayerStates[player_index]->mCue = val;
      emit(player_cue_changed(player_index, val));
   }
}

/*
void AudioModel::set_player_out_state(int player_index, Player::out_state_t val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   //TODO
}

void AudioModel::set_player_stretch_method(int player_index, Player::stretch_method_t val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //TODO
}
*/

void AudioModel::set_player_mute(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mMute != val) {
      Command * cmd = NULL;
      if (val)
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::MUTE);
      else
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::NO_MUTE);
      queue_command(cmd);
      mPlayerStates[player_index]->mMute = val;
      emit(player_mute_changed(player_index, val));
   }
}

void AudioModel::set_player_sync(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mSync != val) {
      Command * cmd = NULL;
      if (val)
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::SYNC);
      else
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::NO_SYNC);
      queue_command(cmd);
      mPlayerStates[player_index]->mSync = val;
      emit(player_sync_changed(player_index, val));
   }
}

void AudioModel::set_player_loop(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mLoop != val) {
      Command * cmd = NULL;
      if (val)
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::LOOP);
      else
         cmd = new DataJockey::Audio::PlayerStateCommand(player_index, 
               DataJockey::Audio::PlayerStateCommand::NO_LOOP);
      queue_command(cmd);
      mPlayerStates[player_index]->mLoop = val;
      emit(player_loop_changed(player_index, val));
   }
}

void AudioModel::set_player_volume(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mVolume != (unsigned int)val) {
      double volume = (double)val / double(one_scale);
      queue_command(new DataJockey::Audio::PlayerDoubleCommand(player_index, 
               DataJockey::Audio::PlayerDoubleCommand::VOLUME, volume));
      mPlayerStates[player_index]->mVolume = val;
      emit(player_volume_changed(player_index, val));
   }
}

void AudioModel::set_player_play_speed(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   
   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mPlaySpeed != (unsigned int)val) {
      double speed = (double)val / double(one_scale);
      queue_command(new DataJockey::Audio::PlayerDoubleCommand(player_index, 
               DataJockey::Audio::PlayerDoubleCommand::PLAY_SPEED, speed));
      mPlayerStates[player_index]->mPlaySpeed = val;
      emit(player_play_speed_changed(player_index, val));
   }
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

void AudioModel::set_player_position_relative(int player_index, const DataJockey::Audio::TimePoint &val) {
   set_player_position(player_index, val, false);
}

void AudioModel::set_player_position(int player_index, double seconds, bool absolute) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   TimePoint timepoint(seconds);

   Command * cmd = NULL;
   if (absolute)
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, timepoint);
   else
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY_RELATIVE, timepoint);
   queue_command(cmd);
}

void AudioModel::set_player_position_relative(int player_index, double seconds) {
   set_player_position(player_index, seconds, false);
}

void AudioModel::set_player_position_frame(int player_index, int frame, bool absolute) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (absolute)
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, frame);
   else
      cmd = new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY_RELATIVE, frame);
   queue_command(cmd);
}

void AudioModel::set_player_position_frame_relative(int player_index, int frame) {
   if (frame != 0)
      set_player_position_frame(player_index, frame, false);
}

int AudioModel::get_player_position_frame(int player_index) {
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;

   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mCurrentFrame;
}

void AudioModel::set_player_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::START, val));
}

void AudioModel::set_player_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::END, val));
}

void AudioModel::set_player_loop_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_START, val));
}

void AudioModel::set_player_loop_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_END, val));
}

void AudioModel::set_player_audio_buffer(int player_index, AudioBuffer * buf){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new PlayerSetAudioBufferCommand(player_index, buf));
}

void AudioModel::set_player_beat_buffer(int player_index, BeatBuffer * /* buf */){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //TODO
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
   if (player_index < 0 || player_index >= (int)mNumPlayers || band < 0 || band > 2)
      return;

   int ione_scale = one_scale;

   QMutexLocker lock(&mPlayerStatesMutex);
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
   switch(band) {
      case 0:
         action = PlayerDoubleCommand::EQ_LOW; break;
      case 1:
         action = PlayerDoubleCommand::EQ_MID; break;
      case 2:
         action = PlayerDoubleCommand::EQ_HIGH; break;
   }

   queue_command(new PlayerDoubleCommand(player_index, action, remaped));

   //TODO emit change
}

void AudioModel::relay_player_audio_file_changed(int player_index, QString fileName){
   emit(player_audio_file_changed(player_index, fileName));
}

void AudioModel::relay_player_position_changed(int player_index, int frame_index){
   emit(player_position_changed(player_index, frame_index));
}

void AudioModel::relay_audio_file_load_progress(QString fileName, int percent){
   QMutexLocker lock(&mPlayerStatesMutex);
   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++) {
      if (mPlayerStates[player_index]->mFileName == fileName)
         emit(player_audio_file_load_progress(player_index, percent));
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


void AudioModel::set_master_volume(int val){
   if (val < 0)
      val = 0;
   else if (val > 1.5 * one_scale)
      val = 1.5 * one_scale;
   //TODO actually implement
   emit(master_volume_changed(val));
}

void AudioModel::set_master_cue_volume(int /*val*/){
   //TODO
}

void AudioModel::set_master_cross_fade_enable(bool enable){
   Command * cmd = NULL;
   if(enable)
      cmd = new MasterBoolCommand(MasterBoolCommand::XFADE);
   else
      cmd = new MasterBoolCommand(MasterBoolCommand::NO_XFADE);
   queue_command(cmd);
}

void AudioModel::set_master_cross_fade_position(int val){
   if (val < 0)
      val = 0;
   else if (val > (int)one_scale)
      val = one_scale;
   double dval = (double)val / (double)one_scale;
   queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, dval));

   emit(master_cross_fade_position_changed(val));
}

void AudioModel::set_master_cross_fade_players(int left, int right){
   if (left < 0 || left >= (int)mNumPlayers)
      return;
   if (right < 0 || right >= (int)mNumPlayers)
      return;
   queue_command(new MasterXFadeSelectCommand((unsigned int)left, (unsigned int)right));
}

void AudioModel::set_master_bpm(double bpm) {
   queue_command(new TransportBPMCommand(mMaster->transport(), bpm));
   emit(master_bpm_changed(bpm));
}

void AudioModel::start_audio() {
   mAudioIO->start();
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

