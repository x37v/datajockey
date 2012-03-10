#include "master.hpp"
#include <math.h>

using namespace DataJockey::Audio;

#define DEFAULT_NUM_PLAYERS 2
Master * Master::cInstance = NULL;

Master * Master::instance(){
   if(cInstance == NULL)
      cInstance = new Master;
   return cInstance;
}

Master::Master(){
   mCueBuffer = NULL;
   /*
      for(unsigned int i = 0; i < DEFAULT_NUM_PLAYERS; i++)
      add_player();
      */
   mMasterVolume = 1.0;
   mCueVolume = 1.0;
   mCueBuffer = NULL;
   mMasterVolumeBuffer = NULL;
   mCrossFadeBuffer = NULL;
   mCrossFadePosition = 0.5;
   mCrossFade = false;
   mCrossFadeMixers[0] = 0;
   mCrossFadeMixers[1] = 1;

   //set up lv2
   mLV2World = slv2_world_new();
   mLV2Plugins = NULL;
   if (mLV2World) {
      slv2_world_load_all(mLV2World);
      mLV2Plugins = slv2_world_get_all_plugins(mLV2World);
   }
}

Master::~Master(){
   cInstance = NULL;
   //clean up!
   if(mCueBuffer != NULL){
      delete [] mCueBuffer[0];
      delete [] mCueBuffer[1];
      delete [] mCueBuffer;
   }
   for(unsigned int i = 0; i < mPlayerBuffers.size(); i++){
      delete [] mPlayerBuffers[i][0];
      delete [] mPlayerBuffers[i][1];
      delete [] mPlayerBuffers[i];
   }
   if(mMasterVolumeBuffer)
      delete [] mMasterVolumeBuffer;
   if(mCrossFadeBuffer){
      delete [] mCrossFadeBuffer[0];
      delete [] mCrossFadeBuffer[1];
      delete [] mCrossFadeBuffer;
   }
   for(unsigned int i = 0; i < mPlayers.size(); i++){
      delete mPlayers[i];
   }
}

void Master::setup_audio(
      unsigned int sampleRate,
      unsigned int maxBufferLen){
   if(mCueBuffer != NULL){
      delete [] mCueBuffer[0];
      delete [] mCueBuffer[1];
      delete [] mCueBuffer;
   }
   mCueBuffer = new float*[2];
   mCueBuffer[0] = new float[maxBufferLen];
   mCueBuffer[1] = new float[maxBufferLen];

   if(mPlayerBuffers.size()){
      for(unsigned int i = 0; i < mPlayerBuffers.size(); i++){
         delete [] mPlayerBuffers[i][0];
         delete [] mPlayerBuffers[i][1];
         delete [] mPlayerBuffers[i];
      }
   }

   //set up the players and their buffers
   for(unsigned int i = 0; i < mPlayers.size(); i++){
      float ** sampleBuffer = new float*[2];
      sampleBuffer[0] = new float[maxBufferLen];
      sampleBuffer[1] = new float[maxBufferLen];
      mPlayers[i]->setup_audio(sampleRate, maxBufferLen);
      for(unsigned int j = 0; j < maxBufferLen; j++) {
         sampleBuffer[0][j] = 0.0f;
         sampleBuffer[1][j] = 0.0f;
      }
      mPlayerBuffers.push_back(sampleBuffer);
   }
   if(mMasterVolumeBuffer)
      delete [] mMasterVolumeBuffer;
   mMasterVolumeBuffer = new float[maxBufferLen];

   if(mCrossFadeBuffer){
      delete [] mCrossFadeBuffer[0];
      delete [] mCrossFadeBuffer[1];
      delete [] mCrossFadeBuffer;
   }
   mCrossFadeBuffer = new float*[2];
   mCrossFadeBuffer[0] = new float[maxBufferLen];
   mCrossFadeBuffer[1] = new float[maxBufferLen];
   mTransport.setup(sampleRate);
}

void Master::add_player(){
   mPlayers.push_back(new Player);
}

void Master::audio_compute_and_fill(
      JackCpp::AudioIO::audioBufVector outBufferVector,
      unsigned int numFrames){

   //execute the schedule
   mScheduler.execute_schedule(mTransport);

   //set up players
   for(unsigned int p = 0; p < mPlayers.size(); p++)
      mPlayers[p]->audio_pre_compute(numFrames, mPlayerBuffers[p], mTransport);

   //compute their samples [and do other stuff]
   for(unsigned int frame = 0; frame < numFrames; frame++){
      //tick the transport
      bool beat = mTransport.tick();
      //XXX this should be a setting
      //only execute every 64 samples, at 44.1khz this is every 1.45ms
      if(frame % 64 == 0){
         //execute the schedule
         mScheduler.execute_schedule(mTransport);
      }
      for(unsigned int chan = 0; chan < 2; chan++){
         //zero out the cue buffer
         mCueBuffer[chan][frame] = 0.0;
         //calculate the crossfade
         if(mCrossFade){
            if(mCrossFadePosition >= 1.0f){
               mCrossFadeBuffer[0][frame] = 0.0;
               mCrossFadeBuffer[1][frame] = 1.0;
            } else if (mCrossFadePosition <= 0.0f){
               mCrossFadeBuffer[0][frame] = 1.0;
               mCrossFadeBuffer[1][frame] = 0.0;
            } else {
               mCrossFadeBuffer[0][frame] = (float)sin((M_PI / 2) * (1.0f + mCrossFadePosition));
               mCrossFadeBuffer[1][frame] = (float)sin((M_PI / 2) * mCrossFadePosition);
            }
         } else {
            mCrossFadeBuffer[0][frame] = mCrossFadeBuffer[1][frame] = 1.0;
         }
      }
      //zero out the output buffers
      for(unsigned int chan = 0; chan < 4; chan++)
         outBufferVector[chan][frame] = 0.0;
      for(unsigned int p = 0; p < mPlayers.size(); p++)
         mPlayers[p]->audio_compute_frame(frame, mPlayerBuffers[p], mTransport, beat);
      //set volume
      mMasterVolumeBuffer[frame] = mMasterVolume;
   }

   //finalize each player, and copy its data out
   for(unsigned int p = 0; p < mPlayers.size(); p++){
      mPlayers[p]->audio_post_compute(numFrames, mPlayerBuffers[p]);
      mPlayers[p]->audio_fill_output_buffers(numFrames, mPlayerBuffers[p], mCueBuffer);

      //actually copy the data to the output, applying cross fade if needed
      if(p == mCrossFadeMixers[0]){
         for(unsigned int frame = 0; frame < numFrames; frame++){
            for(unsigned int chan = 0; chan < 2; chan++){
               outBufferVector[chan][frame] += 
                  mCrossFadeBuffer[0][frame] *
                  mMasterVolumeBuffer[frame] * mPlayerBuffers[p][chan][frame];
               outBufferVector[chan + 2][frame] += mCueVolume * mCueBuffer[chan][frame];
            }
         }
      } else if(p == mCrossFadeMixers[1]){
         for(unsigned int frame = 0; frame < numFrames; frame++){
            for(unsigned int chan = 0; chan < 2; chan++){
               outBufferVector[chan][frame] += 
                  mCrossFadeBuffer[1][frame] *
                  mMasterVolumeBuffer[frame] * mPlayerBuffers[p][chan][frame];
               outBufferVector[chan + 2][frame] += mCueVolume * mCueBuffer[chan][frame];
            }
         }
      } else {
         for(unsigned int frame = 0; frame < numFrames; frame++){
            for(unsigned int chan = 0; chan < 2; chan++){
               outBufferVector[chan][frame] += 
                  mMasterVolumeBuffer[frame] * mPlayerBuffers[p][chan][frame];
               outBufferVector[chan + 2][frame] += mCueVolume * mCueBuffer[chan][frame];
            }
         }
      }
   }
}

//getters
float Master::master_volume() const { return mMasterVolume; }
float Master::cue_volume() const { return mCueVolume; }
bool Master::cross_fadeing() const { return mCrossFade; }
float Master::cross_fade_position() const { return mCrossFadePosition; }
unsigned int Master::cross_fade_mixer(unsigned int index) const {
   if(index > 1)
      return 0;
   else
      return mCrossFadeMixers[index];
}

/*
void Master::sync_to_player(unsigned int player_index) {
}
*/

const std::vector<Player *>& Master::players() const {
   return mPlayers;
}

Scheduler * Master::scheduler(){
   return &mScheduler;
}

Transport * Master::transport(){
   return &mTransport;
}

SLV2World Master::lv2_world() const {
   return mLV2World;
}

SLV2Plugins Master::lv2_plugins() const {
   return mLV2Plugins;
}

//setters
void Master::master_volume(float val){
   mMasterVolume = val;
}

void Master::cue_volume(float val){
   mCueVolume = val;
}

void Master::cross_fade(bool val){
   mCrossFade = val;
}

void Master::cross_fade_position(float val){
   mCrossFadePosition = val;
}

void Master::cross_fade_mixers(unsigned int left, unsigned int right){
   if(left < mPlayers.size() && right < mPlayers.size() && left != right){
      mCrossFadeMixers[0] = left;
      mCrossFadeMixers[1] = right;
   }
}

//commands

MasterCommand::MasterCommand(){
   mMaster = Master::instance();
}

Master * MasterCommand::master() const {
   return mMaster;
}

MasterBoolCommand::MasterBoolCommand(action_t action)
{
   mAction = action;
}

void MasterBoolCommand::execute(){
   switch(mAction){
      case XFADE:
         master()->cross_fade(true);
         break;
      case NO_XFADE:
         master()->cross_fade(false);
         break;
   };
}

bool MasterBoolCommand::store(CommandIOData& /*data*/) const {
   //XXX TODO
   return false;
}

MasterIntCommand::MasterIntCommand(action_t action, int value) : 
   mAction(action), mValue (value){
}

void MasterIntCommand::execute() {
   /*
   switch(mAction) {
      case SYNC_TO_PLAYER:
         master()->sync_to_player(mValue);
         break;
   }
   */
}

bool MasterIntCommand::store(CommandIOData& data) const {
   //XXX TODO
   return false;
}

MasterDoubleCommand::MasterDoubleCommand(action_t action, double val)
{
   mAction = action;
   mValue = val;
}

void MasterDoubleCommand::execute(){
   switch(mAction){
      case MAIN_VOLUME:
         master()->master_volume(mValue);
         break;
      case CUE_VOLUME:
         master()->cue_volume(mValue);
         break;
      case XFADE_POSITION:
         master()->cross_fade_position(mValue);
         break;
   };
}

bool MasterDoubleCommand::store(CommandIOData& /*data*/) const {
   //XXX TODO
   return false;
}

MasterXFadeSelectCommand::MasterXFadeSelectCommand(
      unsigned int left, unsigned int right)
{
   mSel[0] = left;
   mSel[1] = right;
}

void MasterXFadeSelectCommand::execute(){
   master()->cross_fade_mixers(mSel[0], mSel[1]);
}

bool MasterXFadeSelectCommand::store(CommandIOData& /*data*/) const {
   //XXX TODO
   return false;
}

