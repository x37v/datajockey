#include "audiomodel.hpp"

using namespace DataJockey;
using namespace DataJockey::Internal;


AudioLoaderThread::AudioLoaderThread(AudioModel * model, unsigned int player_index) 
: mAudioModel(model), mPlayerIndex(player_index){ }

void AudioLoaderThread::run() {
}

ConsumeThread::ConsumeThread(Scheduler * scheduler) :
   mScheduler(scheduler)
{ }

void ConsumeThread::run() {
   while(true) {
      mScheduler->execute_done_actions();
      msleep(10);
   }
}

const unsigned int DataJockey::AudioModel::one_scale = 1000;
AudioModel * AudioModel::cInstance = NULL;

AudioModel::AudioModel() :
   QObject()
{
   unsigned int num_players = 2;

   mAudioIO = DataJockey::Internal::AudioIO::instance();
   mMaster = DataJockey::Internal::Master::instance();

   mNumPlayers = num_players;
   for(unsigned int i = 0; i < mNumPlayers; i++) {
      mMaster->add_player();
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
   mConsumeThread = new Internal::ConsumeThread(mMaster->scheduler());
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

   AudioBuffer * buf = new AudioBuffer(location.toStdString());
   set_player_audio_buffer(player_index, buf);
}

void AudioModel::queue_command(DataJockey::Internal::Command * cmd){
   mMaster->scheduler()->execute(cmd);
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

