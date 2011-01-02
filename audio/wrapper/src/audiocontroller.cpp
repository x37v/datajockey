#include "audiocontroller.hpp"
#include "audioloaderthread.hpp"
#include "audiobufferreference.hpp"

#include <QMutexLocker>
#include <vector>
#include <QMetaObject>

using namespace DataJockey;
using namespace DataJockey::Audio;

#include <iostream>
using std::cout;
using std::endl;

class AudioController::PlayerClearBuffersCommand : public DataJockey::Audio::PlayerCommand {
   public:
      PlayerClearBuffersCommand(unsigned int idx,
            AudioController * controller,
            QString oldFileName = QString()) :
         DataJockey::Audio::PlayerCommand(idx),
         mAudioController(controller),
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
         if (!mOldFileName.isEmpty()) {
            //low level, decrement reference
            AudioBufferReference::decrement_count(mOldFileName);
         }
      }
      virtual bool store(CommandIOData& /*data*/) const {
         //TODO
         return false;
      }
   private:
      AudioController * mAudioController;
      QString mOldFileName;
};

class AudioController::PlayerState {
   public:
      PlayerState() :
         mFileName(), mCurrentFrame(0) { }
      QString mFileName;
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
class AudioController::QueryPlayerStates : public MasterCommand {
   private:
      AudioController * mAudioController;
   public:
      std::vector<AudioController::PlayerState* > mStates;
      unsigned int mNumPlayers;

      QueryPlayerStates(AudioController * controller) : mAudioController(controller) {
         mNumPlayers = mAudioController->player_count();
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mStates.push_back(new AudioController::PlayerState);
         mStates.resize(mNumPlayers);
      }
      virtual ~QueryPlayerStates() {
         for(unsigned int i = 0; i < mNumPlayers; i++)
            delete mStates[i];
      }
      virtual void execute(){
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mStates[i]->mCurrentFrame = master()->players()[i]->current_frame();
      }
      virtual void execute_done() {
         for(unsigned int i = 0; i < mNumPlayers; i++)
            mAudioController->update_player_state(i, mStates[i]);
      }
      //this command shouldn't be stored
      virtual bool store(CommandIOData& /* data */) const { return false; }
};

class AudioController::ConsumeThread : public QThread {
   private:
      Scheduler * mScheduler;
      AudioController * mController;
   public:
      ConsumeThread(AudioController * controller, Scheduler * scheduler) : mScheduler(scheduler), mController(controller) { }

      void run() {
         while(true) {
            AudioController::QueryPlayerStates * cmd = new AudioController::QueryPlayerStates(mController);
            mScheduler->execute(cmd);
            mScheduler->execute_done_actions();
            msleep(10);
         }
      }
};

const unsigned int DataJockey::Audio::AudioController::one_scale = 1000;
AudioController * AudioController::cInstance = NULL;

AudioController::AudioController() :
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
      player->sync(false);
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

AudioController::~AudioController() {
}

AudioController * AudioController::instance(){
   if (!cInstance)
      cInstance = new AudioController();
   return cInstance;
}

//*************** getters

unsigned int AudioController::player_count() const { return mNumPlayers; }

bool AudioController::player_pause(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mPause;
}

bool AudioController::player_cue(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mCue;
}

//void AudioController::player_out_state(int player_index, Audio::Player::out_state_t val){ } 
//void AudioController::player_stretch_method(int player_index, Audio::Player::stretch_method_t val){ }

bool AudioController::player_mute(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mMute;
}

bool AudioController::player_sync(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mSync;
}

bool AudioController::player_loop(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return false;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mLoop;
}

int AudioController::player_volume(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mVolume;
}

int AudioController::player_play_speed(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return 0;
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mPlaySpeed;
}

//void AudioController::player_position(int player_index){ }
//void AudioController::player_start_position(int player_index){ }
//void AudioController::player_end_position(int player_index){ }
//void AudioController::player_loop_start_position(int player_index){ }
//void AudioController::player_loop_end_position(int player_index){ }

QString AudioController::player_audio_file(int player_index){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return QString();
   QMutexLocker lock(&mPlayerStatesMutex);
   return mPlayerStates[player_index]->mFileName;
}

//****************** setters/slots

void AudioController::set_player_pause(int player_index, bool pause){
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

void AudioController::set_player_cue(int player_index, bool val){
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
void AudioController::set_player_out_state(int player_index, Player::out_state_t val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   //TODO
}

void AudioController::set_player_stretch_method(int player_index, Player::stretch_method_t val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //TODO
}
*/

void AudioController::set_player_mute(int player_index, bool val){
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

void AudioController::set_player_sync(int player_index, bool val){
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

void AudioController::set_player_loop(int player_index, bool val){
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

void AudioController::set_player_volume(int player_index, int val){
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

void AudioController::set_player_play_speed(int player_index, int val){
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

void AudioController::set_player_position(int player_index, const TimePoint &val, bool absolute){
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

void AudioController::set_player_position_relative(int player_index, const DataJockey::Audio::TimePoint &val) {
   set_player_position(player_index, val, false);
}

void AudioController::set_player_position(int player_index, double seconds, bool absolute) {
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

void AudioController::set_player_position_relative(int player_index, double seconds) {
   set_player_position(player_index, seconds, false);
}

void AudioController::set_player_position_frame(int /* player_index */, unsigned long /*frame*/, bool /*absolute*/) {
}

void AudioController::set_player_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::START, val));
}

void AudioController::set_player_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::END, val));
}

void AudioController::set_player_loop_start_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_START, val));
}

void AudioController::set_player_loop_end_position(int player_index, const TimePoint &val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new DataJockey::Audio::PlayerPositionCommand(
            player_index, PlayerPositionCommand::LOOP_END, val));
}

void AudioController::set_player_audio_buffer(int player_index, AudioBuffer * buf){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   queue_command(new PlayerLoadCommand(player_index, buf));
}

void AudioController::set_player_beat_buffer(int player_index, BeatBuffer * /* buf */){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   //TODO
}

void AudioController::update_player_state(int player_index, PlayerState * state){
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

void AudioController::set_player_audio_file(int player_index, QString location){
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
            mThreadPool[player_index]->load(location);
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

void AudioController::set_player_clear_buffers(int player_index) {
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

void AudioController::relay_player_audio_file_changed(int player_index, QString fileName){
   emit(player_audio_file_changed(player_index, fileName));
}

void AudioController::relay_player_position_changed(int player_index, int frame_index){
   emit(player_position_changed(player_index, frame_index));
}

void AudioController::relay_audio_file_load_progress(QString fileName, int percent){
   QMutexLocker lock(&mPlayerStatesMutex);
   for(unsigned int player_index = 0; player_index < mPlayerStates.size(); player_index++) {
      if (mPlayerStates[player_index]->mFileName == fileName)
         emit(player_audio_file_load_progress(player_index, percent));
   }
}

//called from another thread
bool AudioController::audio_file_load_complete(QString fileName, AudioBuffer * buffer){
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

void AudioController::set_master_volume(int /*val*/){
   //TODO
}

void AudioController::set_master_cue_volume(int /*val*/){
   //TODO
}

void AudioController::set_master_cross_fade_enable(bool enable){
   Command * cmd = NULL;
   if(enable)
      cmd = new MasterBoolCommand(MasterBoolCommand::XFADE);
   else
      cmd = new MasterBoolCommand(MasterBoolCommand::NO_XFADE);
   queue_command(cmd);
}

void AudioController::set_master_cross_fade_position(int val){
   double dval = (double)val / (double)one_scale;
   if (dval > 1.0)
      dval = 1.0;
   else if(dval < 0.0)
      dval = 0.0;
   queue_command(new MasterDoubleCommand(MasterDoubleCommand::XFADE_POSITION, dval));
}

void AudioController::set_master_cross_fade_players(int left, int right){
   if (left < 0 || left >= (int)mNumPlayers)
      return;
   if (right < 0 || right >= (int)mNumPlayers)
      return;
   queue_command(new MasterXFadeSelectCommand((unsigned int)left, (unsigned int)right));
}

void AudioController::start_audio() {
   mAudioIO->start();
   mAudioIO->connectToPhysical(0,0);
   mAudioIO->connectToPhysical(1,1);
}

void AudioController::stop_audio() {
   mAudioIO->stop();
}

void AudioController::queue_command(DataJockey::Audio::Command * cmd){
   mMaster->scheduler()->execute(cmd);
}

