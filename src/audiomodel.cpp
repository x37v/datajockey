#include "audiomodel.hpp"
#include <QMutexLocker>

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

AudioLoaderThread::AudioLoaderThread(AudioModel * model, unsigned int player_index) 
: mAudioModel(model), mPlayerIndex(player_index){ }

void AudioLoaderThread::run() {
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
      mThreadPool.push_back(new AudioLoaderThread(this, i));
   }

   for(unsigned int i = 0; i < mNumPlayers; i++) {
      mMaster->players()[i]->sync(false);
      mMaster->players()[i]->out_state(Player::MAIN_MIX);
      mMaster->players()[i]->play_state(Player::PLAY);
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

   Command * cmd = NULL;
   if (pause)
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::PAUSE);
   else
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::PLAY);
   queue_command(cmd);
}

void AudioModel::set_player_cue(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (val)
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::OUT_CUE);
   else
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, PlayerStateCommand::OUT_MAIN);
   queue_command(cmd);
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

   Command * cmd = NULL;
   if (val)
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::MUTE);
   else
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::NO_MUTE);
   queue_command(cmd);
}

void AudioModel::set_player_sync(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (val)
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::SYNC);
   else
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::NO_SYNC);
   queue_command(cmd);
}

void AudioModel::set_player_loop(int player_index, bool val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   Command * cmd = NULL;
   if (val)
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::LOOP);
   else
      cmd = new DataJockey::Internal::PlayerStateCommand(player_index, 
            DataJockey::Internal::PlayerStateCommand::NO_LOOP);
   queue_command(cmd);
}

void AudioModel::set_player_volume(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;

   double volume = (double)val / double(one_scale);
   queue_command(new DataJockey::Internal::PlayerDoubleCommand(player_index, 
            DataJockey::Internal::PlayerDoubleCommand::VOLUME, volume));
}

void AudioModel::set_player_play_speed(int player_index, int val){
   if (player_index < 0 || player_index >= (int)mNumPlayers)
      return;
   
   double speed = (double)val / double(one_scale);
   queue_command(new DataJockey::Internal::PlayerDoubleCommand(player_index, 
            DataJockey::Internal::PlayerDoubleCommand::PLAY_SPEED, speed));
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

      //notify
      emit(player_audio_file_changed(player_index, location));

      if (!mAudioBufferManager.contains(location)) {
         buf = new AudioBuffer(location.toStdString());
         set_player_audio_buffer(player_index, buf);

         //update the manager
         mAudioBufferManager[location] = QPair<int, AudioBuffer *>(1, buf);

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
         //XXX what if the file isn't actually all the way loaded?
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

void AudioModel::relay_player_audio_file_load_progress(int player_index, int percent){
   emit(player_audio_file_load_progress(player_index, percent));
}

void AudioModel::set_master_volume(int val){
}

void AudioModel::set_master_cue_volume(int val){
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

