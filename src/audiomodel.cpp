#include "audiomodel.hpp"
#include <QMutexLocker>
#include <QMetaObject>

using namespace DataJockey;
using namespace DataJockey::Internal;

#include <iostream>
using std::cout;
using std::endl;

class AudioModel::PlayerClearBuffersCommand : public DataJockey::Internal::PlayerCommand {
   public:
      PlayerClearBuffersCommand(unsigned int idx,
            AudioModel * model,
            QString oldFileName = QString()) :
         DataJockey::Internal::PlayerCommand(idx),
         mAudioModel(model),
         mOldFileName(oldFileName) { }
      virtual ~PlayerClearBuffersCommand() { }
      virtual void execute() {
         Player * p = player(); 
         if(p != NULL){
            p->audio_buffer(NULL);
            p->beat_buffer(NULL);
         }
      }
      virtual void execute_done() {
         if (!mOldFileName.isEmpty() && mAudioModel != NULL)
            mAudioModel->decrement_audio_file_reference(mOldFileName);
      }
      virtual bool store(CommandIOData& data) const {
         //TODO
         return false;
      }
   private:
      AudioModel * mAudioModel;
      QString mOldFileName;
};

AudioLoaderThread::AudioLoaderThread(AudioModel * model)
: mAudioModel(model), mFileName(), mAudioBuffer(NULL), mMutex(QMutex::Recursive) { }

void AudioLoaderThread::progress_callback(int percent, void *objPtr) {
   AudioLoaderThread * self = (AudioLoaderThread *)objPtr;
   QMetaObject::invokeMethod(self->model(),
         "relay_player_audio_file_load_progress",
         Qt::QueuedConnection, 
         Q_ARG(QString, self->file_name()),
         Q_ARG(int, percent));
}

void AudioLoaderThread::abort() {
   QMutexLocker lock(&mMutex);
   if (mAudioBuffer)
      mAudioBuffer->abort_load();
   mAborted = true;
}

const QString& AudioLoaderThread::file_name() {
   QMutexLocker lock(&mMutex);
   return mFileName;
}

AudioBuffer * AudioLoaderThread::load(QString location){
   QMutexLocker lock(&mMutex);
   mAborted = false;

   if (isRunning()) {
      abort();
      wait();
      //TODO what if it is loading?  how do we deal with a hanging reference?
   }
   mFileName = location;

   try {
      mAudioBuffer = new DataJockey::AudioBuffer(location.toStdString());
      start();
   } catch (...){ return NULL; }
   return mAudioBuffer;
}

void AudioLoaderThread::run() {
   if (mAudioBuffer) {
      mAudioBuffer->load(AudioLoaderThread::progress_callback, this);
      {
         QMutexLocker lock(&mMutex);
         if (!mAborted) {
            //tell the model that the load is complete.
            //if it doesn't use the buffer then delete it
            if (!model()->audio_file_load_complete(mFileName, mAudioBuffer))
               delete mAudioBuffer;
         } else
            delete mAudioBuffer;
         //cleanup
         mAudioBuffer = NULL;
         mFileName = QString();
      }
   }
}

AudioModel * AudioLoaderThread::model() {
   return mAudioModel;
}

class DataJockey::AudioModel::ConsumeThread : public QThread {
   private:
      Scheduler * mScheduler;
   public:
      ConsumeThread(Scheduler * scheduler) : mScheduler(scheduler) { }

      void run() {
         while(true) {
            mScheduler->execute_done_actions();
            msleep(10);
         }
      }
};

class DataJockey::AudioModel::PlayerState {
   public:
      PlayerState() :
         mFileName() { }
      QString mFileName;
      unsigned int mVolume;
      unsigned int mPlaySpeed;
      bool mMute;
      bool mSync;
      bool mLoop;
      bool mCue;
      bool mPause;
};

const unsigned int DataJockey::AudioModel::one_scale = 1000;
AudioModel * AudioModel::cInstance = NULL;

AudioModel::AudioModel() :
   QObject(),
   mPlayerStates(),
   mPlayerStatesMutex(QMutex::Recursive)
{
   unsigned int num_players = 2;

   mAudioIO = DataJockey::Internal::AudioIO::instance();
   mMaster = DataJockey::Internal::Master::instance();

   mNumPlayers = num_players;
   for(unsigned int i = 0; i < mNumPlayers; i++) {
      mMaster->add_player();
      mPlayerStates.push_back(new PlayerState());
      mThreadPool.push_back(new AudioLoaderThread(this));
   }

   for(unsigned int i = 0; i < mNumPlayers; i++) {
      DataJockey::Internal::Player * player = mMaster->players()[i];
      player->sync(false);
      player->out_state(Player::MAIN_MIX);
      player->play_state(Player::PLAY);

      //init player states
      //bool
      mPlayerStates[i]->mMute = player->muted();
      mPlayerStates[i]->mSync = player->syncing();
      mPlayerStates[i]->mLoop = player->looping();
      mPlayerStates[i]->mCue = (player->out_state() == DataJockey::Internal::Player::CUE);
      mPlayerStates[i]->mPause = (player->play_state() == DataJockey::Internal::Player::PAUSE);

      //int
      mPlayerStates[i]->mVolume = one_scale * player->volume();
      mPlayerStates[i]->mPlaySpeed = one_scale * player->play_speed();
   }

   mAudioIO->start();
   mAudioIO->connectToPhysical(0,0);
   mAudioIO->connectToPhysical(1,1);

   //hook up and start the consume thread
   mConsumeThread = new ConsumeThread(mMaster->scheduler());
   mConsumeThread->start();
}

AudioModel * AudioModel::instance(){
   if (!cInstance)
      cInstance = new AudioModel();
   return cInstance;
}

AudioModel::~AudioModel() {
}

void AudioModel::set_player_pause(int player_index, bool pause){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mPause != pause) {
      Command * cmd = NULL;
      if (pause)
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::PAUSE);
      else
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::PLAY);
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
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::OUT_CUE);
      else
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::OUT_MAIN);
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
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::MUTE);
      else
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::NO_MUTE);
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
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::SYNC);
      else
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::NO_SYNC);
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
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::LOOP);
      else
         cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
               DataJockey::Internal::PlayerStateCommand::NO_LOOP);
      queue_command(cmd);
      mPlayerStates[player_index]->mLoop = val;
      emit(player_loop_changed(player_index, val));
   }
}

void AudioModel::set_player_volume(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mVolume != val) {
      double volume = (double)val / double(one_scale);
      queue_command(new DataJockey::Internal::PlayerDoubleCommand(player_index, 
               DataJockey::Internal::PlayerDoubleCommand::VOLUME, volume));
      mPlayerStates[player_index]->mVolume = val;
      emit(player_volume_changed(player_index, val));
   }
}

void AudioModel::set_player_play_speed(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   
   QMutexLocker lock(&mPlayerStatesMutex);
   if (mPlayerStates[player_index]->mPlaySpeed != val) {
      double speed = (double)val / double(one_scale);
      queue_command(new DataJockey::Internal::PlayerDoubleCommand(player_index, 
               DataJockey::Internal::PlayerDoubleCommand::PLAY_SPEED, speed));
      mPlayerStates[player_index]->mPlaySpeed = val;
      emit(player_play_speed_changed(player_index, val));
   }
}

void AudioModel::set_player_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Internal::PlayerPositionCommand(
            player_index, PlayerPositionCommand::PLAY, val));
}

void AudioModel::set_player_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Internal::PlayerPositionCommand(
            player_index, PlayerPositionCommand::START, val));
}

void AudioModel::set_player_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Internal::PlayerPositionCommand(
            player_index, PlayerPositionCommand::END, val));
}

void AudioModel::set_player_loop_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Internal::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_START, val));
}

void AudioModel::set_player_loop_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Internal::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_END, val));
}

void AudioModel::set_player_audio_buffer(int player_index, AudioBuffer * buf){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new PlayerLoadCommand(player_index, buf));
}

void AudioModel::set_player_beat_buffer(int player_index, BeatBuffer * buf){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //TODO
}

void AudioModel::set_player_audio_file(int player_index, QString location){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   QMutexLocker lock(&mPlayerStatesMutex);

   //only update if new
   QString oldfile = mPlayerStates[player_index]->mFileName;
   if (oldfile != location) {
      AudioBuffer * buf = NULL;

      //clear out the old buffers
      set_player_clear_buffers(player_index);

      //once the manager contains the location we know that it is full loaded
      //but, if not it could actually be in progress
      if (!mAudioBufferManager.contains(location)) {
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
            mThreadPool[player_index]->load(location);
         }

         //update player state
         mPlayerStates[player_index]->mFileName = location;

      } else {
         //increase the reference count
         mAudioBufferManager[location].first += 1;
         buf = mAudioBufferManager[location].second;

         //send the stored buf
         set_player_audio_buffer(player_index, buf);

         //update player state
         mPlayerStates[player_index]->mFileName = location;

         //notify
         emit(player_audio_file_load_progress(player_index, 100));
      }
   } else {
      //XXX what if the file isn't actually all the way loaded?
      emit(player_audio_file_load_progress(player_index, 100));
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

void AudioModel::decrement_audio_file_reference(QString fileName) {
   QMutexLocker lock(&mPlayerStatesMutex);

   if (mAudioBufferManager.contains(fileName)) {
      mAudioBufferManager[fileName].first -= 1;
      if (mAudioBufferManager[fileName].first < 1) {
         delete mAudioBufferManager[fileName].second;
         mAudioBufferManager.remove(fileName);
      }
   }
}

void AudioModel::relay_player_audio_file_load_progress(QString fileName, int percent){
   QMutexLocker lock(&mPlayerStatesMutex);
   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++) {
      if (mPlayerStates[player_index]->mFileName == fileName)
         emit(player_audio_file_load_progress(player_index, percent));
   }
}

void AudioModel::relay_player_audio_file_changed(int player_index, QString fileName) {
   emit(player_audio_file_changed(player_index, fileName));
}

//called from another thread
bool AudioModel::audio_file_load_complete(QString fileName, DataJockey::AudioBuffer * buffer){
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
      if (mAudioBufferManager.contains(fileName))
         mAudioBufferManager[fileName].first += 1;
      else
         mAudioBufferManager[fileName] = QPair<int, AudioBuffer *>(1, buffer);
      loaded_into_a_player = true;

      set_player_audio_buffer(player_index, buffer);

      //reset player position
      set_player_position(player_index, TimePoint(0.0));

      QMetaObject::invokeMethod(this,
            "relay_player_audio_file_changed",
            Qt::QueuedConnection, 
            Q_ARG(int, player_index),
            Q_ARG(QString, fileName));
   }

   if (!loaded_into_a_player)
      return false;
   return true;
}

void AudioModel::set_master_volume(int val){
   //TODO
}

void AudioModel::set_master_cue_volume(int val){
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
   double dval = (double)val / (double)one_scale;
   if (dval > 1.0)
      dval = 1.0;
   else if(dval < 0.0)
      dval = 0.0;
   queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, dval));
}

void AudioModel::set_master_cross_fade_players(int left, int right){
   if (left < 0 || left >= (int)mNumPlayers)
      return;
   if (right < 0 || right >= (int)mNumPlayers)
      return;
   queue_command(new MasterXFadeSelectCommand((unsigned int)left, (unsigned int)right));
}

void AudioModel::queue_command(DataJockey::Internal::Command * cmd){
   mMaster->scheduler()->execute(cmd);
}

