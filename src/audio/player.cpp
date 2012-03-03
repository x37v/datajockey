#include "player.hpp"
#include <math.h>
#include "master.hpp"
#include "stretcher.hpp"
#include "stretcher_rate.hpp"
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define DJ_EQ_URI "http://plugin.org.uk/swh-plugins/dj_eq"

#include <iostream>
using std::cerr;
using std::endl;

using namespace DataJockey::Audio;

Player::Player() : 
   mPosition(0.0),
   mTransportOffset(0,0),
   mEqInstance(NULL)
{
   //states
   mPlayState = PAUSE;
   mOutState = CUE;
   mMute = false;
   mSync = false;
   mLoop = false;

   //continuous
   mVolume = 1.0;

   mVolumeBuffer = NULL;
   mBeatBuffer = NULL;

   //XXX who should keep track of these stretchers?
   mStretcher = new StretcherRate();

   //by default we start at the beginning of the audio
   mStartPosition.at_bar(0);

   mPositionDirty = false;
   mSetup = false;

   mUpdateTransportOffset = true;
}

Player::~Player(){
   //cleanup
   if(mVolumeBuffer)
      delete [] mVolumeBuffer;
	if(mEqInstance)
		slv2_instance_free(mEqInstance);
}

//#error "GO OVER ALL OF THIS!!!!!!!!!!!!!!, stretcher not fully integrated"

//this creates internal buffers
//** must be called BEFORE the audio callback starts
void Player::setup_audio(
      unsigned int sampleRate,
      unsigned int maxBufferLen){
   //set the sample rate, create our internal audio buffers
   mSampleRate = sampleRate;
   if(mVolumeBuffer)
      delete [] mVolumeBuffer;
   mVolumeBuffer = new float[maxBufferLen];

   //do lv2
   Master * master = Master::instance();
	SLV2Value plugin_uri = slv2_value_new_uri(master->lv2_world(), DJ_EQ_URI);
	SLV2Plugin eq_plugin = slv2_plugins_get_by_uri(master->lv2_plugins(), plugin_uri);
   if (!eq_plugin) {
		cerr << "could not load eq lv2 plugin, do you have dj eq installed?:" << endl;
		cerr << "\t\t" << DJ_EQ_URI << endl;
   } else {
      //load the plugin
      mEqInstance = slv2_plugin_instantiate(eq_plugin, mSampleRate, NULL);
      if (!mEqInstance) {
         cerr << "could not instantiate eq lv2 plugin, do you have dj eq installed?:" << endl;
         cerr << "\t\t" << DJ_EQ_URI << endl;
      } else {
         slv2_instance_connect_port(mEqInstance, 0, &mEqControl.low);
         slv2_instance_connect_port(mEqInstance, 1, &mEqControl.mid);
         slv2_instance_connect_port(mEqInstance, 2, &mEqControl.high);
         slv2_instance_connect_port(mEqInstance, 7, &mEqControl.latency);
         slv2_instance_activate(mEqInstance);
      }
   }

   mSetup = true;
}

//the audio computation methods
//setup for audio computation
void Player::audio_pre_compute(unsigned int numFrames, float ** mixBuffer, const Transport& transport){ 

   if(!mStretcher->audio_buffer())
      return;

   //do we need to update the mSampleIndex based on the position?
   if(mPositionDirty)
      update_position(transport);

   if(mUpdateTransportOffset) {
      update_transport_offset(transport);
      if (mBeatBuffer && mSync)
         update_play_speed(transport);
   }

   if(mStretcher->frame() >= mStretcher->audio_buffer()->length())
      return;
   if(mEndPosition.valid() && mPosition >= mEndPosition)
      return;
}

//actually compute one frame, filling an internal buffer
//syncing to the transport if mSync == true
void Player::audio_compute_frame(unsigned int frame, float ** mixBuffer, 
      const Transport& transport, bool inbeat){
   //zero out the frame;
   mixBuffer[0][frame] = mixBuffer[1][frame] = 0.0;

   if(!mStretcher->audio_buffer())
      return;

   //compute the volume
   mVolumeBuffer[frame] = mMute ? 0.0 : mVolume;

   //compute the actual frame
   if(mPlayState == PLAY){
      if(mUpdateTransportOffset) {
         update_transport_offset(transport);
         mPosition = transport.position() - mTransportOffset;
         update_position(transport);
      }

      //only update the rate on the beat.
      if(inbeat && mSync && mBeatBuffer){
         mPosition = transport.position() - mTransportOffset;
         update_position(transport);
         update_play_speed(transport);
         //do we need to update the mSampleIndex based on the position?
      } else if(mPositionDirty)
         update_position(transport);

      float buffer[2];
      mStretcher->next_frame(buffer);
      mixBuffer[0][frame] = buffer[0];
      mixBuffer[1][frame] = buffer[1];
   }

#if 0
      //XXX what to do?
      //update our position
      if(mBeatBuffer){
         mPosition = mBeatBuffer->position_at_time(
               ((double)mSampleIndex + mSampleIndexResidual) / (double)mSampleRate, mPosition);
         //if we've reached the end then stop, if we've reached the end of the loop
         //reposition to the beginning of the loop
         if(mEndPosition.valid() && mPosition >= mEndPosition)
            break;
         else if(mLoop && mLoopEndPosition.valid() && 
               mLoopStartPosition.valid() &&
               mPosition >= mLoopEndPosition) {
            position(mLoopStartPosition);
         }
      } else {
         //XXX deal with position if there is no beat buffer
         if (mPosition.type() == TimePoint::SECONDS) {
            double seconds = ((double)mSampleIndex + mSampleIndexResidual) / (double)mSampleRate;
            mPosition.seconds(seconds);
         } else {
            //TODO what?
         }
      }
#endif
}

//finalize audio computation, apply effects, etc.
void Player::audio_post_compute(unsigned int numFrames, float ** mixBuffer){
   if(!mStretcher->audio_buffer())
      return;
   if(mEqInstance) {
      slv2_instance_connect_port(mEqInstance, 3, mixBuffer[0]);
      slv2_instance_connect_port(mEqInstance, 4, mixBuffer[1]);
      slv2_instance_connect_port(mEqInstance, 5, mixBuffer[0]);
      slv2_instance_connect_port(mEqInstance, 6, mixBuffer[1]);
      slv2_instance_run(mEqInstance, numFrames);
   }
}

//actually fill the output vectors
void Player::audio_fill_output_buffers(unsigned int numFrames,
      float ** mixBuffer, float ** cueBuffer){
   if(!mStretcher->audio_buffer())
      return;

   //send the data out, copying to the cue buffer before volume if needed
   if(mOutState == CUE){
      for(unsigned int i = 0; i < 2; i++){
         for(unsigned int j = 0; j < numFrames; j++){
            cueBuffer[i][j] = mixBuffer[i][j];
            mixBuffer[i][j] *= mVolumeBuffer[j];
         }
      }
   } else {
      for(unsigned int i = 0; i < 2; i++){
         for(unsigned int j = 0; j < numFrames; j++) {
            cueBuffer[i][j] = 0.0f;
            mixBuffer[i][j] *= mVolumeBuffer[j];
         }
      }
   }
}

//getters
Player::play_state_t Player::play_state() const { return mPlayState; }
Player::out_state_t Player::out_state() const { return mOutState; }
bool Player::muted() const { return mMute; }
bool Player::syncing() const { return mSync; }
bool Player::looping() const { return mLoop; }
double Player::volume() const { return mVolume; }
double Player::play_speed() const { return mStretcher->speed(); }
const TimePoint& Player::position() const { return mPosition; }
const TimePoint& Player::start_position() const { return mStartPosition; }
const TimePoint& Player::end_position() const { return mEndPosition; }
const TimePoint& Player::loop_start_position() const { return mLoopStartPosition; }
const TimePoint& Player::loop_end_position() const { return mLoopEndPosition; }
unsigned int Player::frame() const { return (!mStretcher->audio_buffer()) ? 0 : mStretcher->frame(); }
AudioBuffer * Player::audio_buffer() const { return mStretcher->audio_buffer(); }
BeatBuffer * Player::beat_buffer() const { return mBeatBuffer; }

//setters
void Player::play_state(play_state_t val){
   if (mPlayState != val) {
      mPlayState = val;
      mUpdateTransportOffset = true;
   }
}

void Player::out_state(out_state_t val){
   mOutState = val;
}

void Player::mute(bool val){
   mMute = val;
}

void Player::sync(bool val){
   if (val != mSync) {
      mSync = val;
      mUpdateTransportOffset = true;
   }
   //TODO change position type?
}

void Player::loop(bool val){
   mLoop = val;
}

void Player::volume(double val){
   mVolume = val;
}

void Player::play_speed(double val){
   mStretcher->speed(val);
}

void Player::position(const TimePoint &val){
   mPosition = val;
   mUpdateTransportOffset = true;

   //if we're not set up then we cannot update our sample index because we don't
   //know the sample rate, so we just set dirty = true
   if(!mSetup){
      mPositionDirty = true;
      return;
   }

   //update sample index
   //if we don't have a beat buffer
   //we should either set to the value [in seconds] if it a TimePoint::SECONDS
   //or if not, we should use the transport rate to convert it to seconds from
   //bars + beats [which we can only do while playing so we set mPositionDirty
   //= true]
   //otherwise we grab the time from the beat buffer!
   mPositionDirty = false;
   if(mPosition.type() == TimePoint::SECONDS){
      unsigned int frame = (double)mSampleRate * mPosition.seconds();
      mStretcher->frame(frame);
   } else if(mBeatBuffer){
      unsigned int frame = mSampleRate * mBeatBuffer->time_at_position(mPosition);
      mStretcher->frame(frame);
   } else
      mPositionDirty = true;

}

void Player::position_at_frame(unsigned long frame) {
   if (!mStretcher->audio_buffer())
      return;

   mStretcher->frame(frame);

   if (mBeatBuffer) {
      double time = mStretcher->frame();
      time /= mSampleRate;
      mPosition = mBeatBuffer->position_at_time(time, mPosition);
   }

   mPositionDirty = false;
}

void Player::start_position(const TimePoint &val){
   mStartPosition = val;
}

void Player::end_position(const TimePoint &val){
   mEndPosition = val;
}

void Player::loop_start_position(const TimePoint &val){
   mLoopStartPosition = val;
}

void Player::loop_end_position(const TimePoint &val){
   mLoopEndPosition = val;
}

void Player::audio_buffer(AudioBuffer * buf){
   mStretcher->audio_buffer(buf);
   //set at the start
   //TODO what if the start position's type is not the same as the position type?
   //mPosition = mStartPosition;
}

void Player::beat_buffer(BeatBuffer * buf){
   mBeatBuffer = buf;
   //set at the start
   //TODO what if the start position's type is not the same as the position type?
   if (mBeatBuffer)
      mPosition = mStartPosition;
   else {
      mPosition.type(TimePoint::SECONDS);
      mPosition.seconds(0.0);
   }
}

void Player::eq(eq_band_t band, double value) {
   if (value < -70.0)
      value = -70.0;
   else if (value > 6.0)
      value = 6.0;
   switch (band) {
      case LOW:
         mEqControl.low = value; break;
      case MID:
         mEqControl.mid = value; break;
      case HIGH:
         mEqControl.high = value; break;
   }
}


//misc
void Player::position_relative(TimePoint amt){
   position(mPosition + amt);
}

void Player::position_at_frame_relative(long offset){
   long frame = offset + mStretcher->frame();
   if (frame > 0)
      mStretcher->frame(offset + mStretcher->frame());
   else
      mStretcher->frame(0);
}

void Player::play_speed_relative(double amt){
   mStretcher->speed(mStretcher->speed() + amt);
}

void Player::volume_relative(double amt){
   mVolume += amt;
}


void Player::update_position(const Transport& transport){
   mPositionDirty = false;
   //XXX do we need this?
#if 0

   //if the type is seconds then set to that value
   //if it isn't and we have a beat buffer set to the appropriate value
   //otherwise guess the position based on the bar/beat and the current transport
   //tempo
   if(mPosition.type() == TimePoint::SECONDS){
      mSampleIndex = mSampleRate * mPosition.seconds();
      mSampleIndexResidual = 0;
   } else if(mBeatBuffer){
      mSampleIndex = mSampleRate * mBeatBuffer->time_at_position(mPosition);
      mSampleIndexResidual = 0;
   } else {
      mSampleIndex = (double)mSampleRate * ((60.0 / transport.bpm()) *
            (double)(mPosition.bar() * mPosition.beats_per_bar() + mPosition.beat()));
      mSampleIndexResidual = 0;
   }
#endif
}

void Player::update_play_speed(const Transport& transport) {
   double secTillBeat = transport.seconds_till_next_beat();
   if (secTillBeat <= 0)
      return;

   TimePoint next = mPosition;
   next.advance_beat();

   double newSpeed = mBeatBuffer->time_at_position(next) - mBeatBuffer->time_at_position(mPosition);
   newSpeed /= secTillBeat; 

   //XXX should make this a setting
   //should we at least set no sync and notify observers?
   //also, if we do abs(newSpeed) we could allow for going backwards?
   if(newSpeed > 0.25 && newSpeed < 4)
      mStretcher->speed(newSpeed);
}

void Player::update_transport_offset(const Transport& transport) {
   mTransportOffset = (transport.position() - mPosition);

   //XXX do we really want to do this?
   if(mTransportOffset.pos_in_beat() > 0.5)
      mTransportOffset.advance_beat();
   mTransportOffset.pos_in_beat(0.0);

   mUpdateTransportOffset = false;
}

//command stuff
//
PlayerCommand::PlayerCommand(unsigned int idx){
   mIndex = idx;
}

unsigned int PlayerCommand::index() const { return mIndex; }
const TimePoint& PlayerCommand::position_executed() const { return mPositionExecuted; }
void PlayerCommand::position_executed(TimePoint const & t){
   mPositionExecuted = t;
}

void PlayerCommand::store(CommandIOData& data, const std::string& name) const {
   data["name"] = name;
   data["player"] = (int)index();
}

Player * PlayerCommand::player(){
   std::vector<Player *> players = Master::instance()->players();
   if(players.size() <= mIndex)
      return NULL;
   else
      return players[mIndex];
}

PlayerStateCommand::PlayerStateCommand(unsigned int idx, action_t action) :
   PlayerCommand(idx)
{
   mAction = action;
}

void PlayerStateCommand::execute(){
   Player * p = player(); 
   if(p != NULL){
      //store the time executed
      position_executed(p->position());
      //execute the action
      switch(mAction){
         case PLAY:
            p->play_state(Player::PLAY);
            break;
         case PAUSE:
            p->play_state(Player::PAUSE);
            break;
         case OUT_MAIN:
            p->out_state(Player::MAIN_MIX);
            break;
         case OUT_CUE:
            p->out_state(Player::CUE);
            break;
         case SYNC:
            p->sync(true);
            break;
         case NO_SYNC:
            p->sync(false);
            break;
         case MUTE:
            p->mute(true);
            break;
         case NO_MUTE:
            p->mute(false);
            break;
         case LOOP:
            p->loop(true);
            break;
         case NO_LOOP:
            p->loop(false);
            break;
      };
   }
}

bool PlayerStateCommand::store(CommandIOData& data) const{
   PlayerCommand::store(data, "PlayerStateCommand");
   switch(mAction){
      case PLAY:
         data["action"] = "play";
         break;
      case PAUSE:
         data["action"] = "pause";
         break;
      case OUT_MAIN:
         data["action"] = "out_main";
         break;
      case OUT_CUE:
         data["action"] = "out_cue";
         break;
      case SYNC:
         data["action"] = "sync";
         break;
      case NO_SYNC:
         data["action"] = "no_sync";
         break;
      case MUTE:
         data["action"] = "mute";
         break;
      case NO_MUTE:
         data["action"] = "no_mute";
         break;
      case LOOP:
         data["action"] = "loop";
         break;
      case NO_LOOP:
         data["action"] = "no_loop";
         break;
   };
   return true;
}

PlayerDoubleCommand::PlayerDoubleCommand(unsigned int idx, 
      action_t action, double value) :
   PlayerCommand(idx)
{
   mAction = action;
   mValue = value;

}

void PlayerDoubleCommand::execute(){
   Player * p = player(); 
   if(p != NULL){
      //store the time executed
      position_executed(p->position());
      //execute the action
      switch(mAction){
         case VOLUME:
            p->volume(mValue); break;
         case VOLUME_RELATIVE:
            p->volume_relative(mValue); break;
         case PLAY_SPEED:
            p->play_speed(mValue); break;
         case PLAY_SPEED_RELATIVE:
            p->play_speed_relative(mValue); break;
         case EQ_LOW:
            p->eq(Player::LOW, mValue); break;
         case EQ_MID:
            p->eq(Player::MID, mValue); break;
         case EQ_HIGH:
            p->eq(Player::HIGH, mValue); break;
      };
   }
}

bool PlayerDoubleCommand::store(CommandIOData& data) const{
   PlayerCommand::store(data, "PlayerDoubleCommand");
   switch(mAction){
      case VOLUME:
         data["action"] = "volume";
         break;
      case VOLUME_RELATIVE:
         data["action"] = "volume_relative";
         break;
      case PLAY_SPEED:
         data["action"] = "play_speed";
         break;
      case PLAY_SPEED_RELATIVE:
         data["action"] = "play_speed_relative";
         break;
      case EQ_LOW:
         data["action"] = "eq_low";
         break;
      case EQ_MID:
         data["action"] = "eq_mid";
         break;
      case EQ_HIGH:
         data["action"] = "eq_high";
         break;
   };
   data["value"] = mValue;
   return true;
}

PlayerSetAudioBufferCommand::PlayerSetAudioBufferCommand(unsigned int idx, 
      AudioBuffer * buffer,
      bool deleteOldBuffer) : 
   PlayerCommand(idx), 
   mBuffer(buffer),
   mOldBuffer(NULL),
   mDeleteOldBuffer(deleteOldBuffer)
{ }

PlayerSetAudioBufferCommand::~PlayerSetAudioBufferCommand() {
   if (mOldBuffer && mDeleteOldBuffer) {
      delete mOldBuffer;
      mOldBuffer = NULL;
   }
}

void PlayerSetAudioBufferCommand::execute(){
   Player * p = player(); 
   if(p != NULL){
      //store the time executed
      position_executed(p->position());
      //store the old buffer pointer
      mOldBuffer = p->audio_buffer();
      //execute the action
      p->audio_buffer(mBuffer);
   }
}

void PlayerSetAudioBufferCommand::execute_done(){
   if (mOldBuffer && mDeleteOldBuffer) {
      delete mOldBuffer;
      mOldBuffer = NULL;
   }
   //execute our super class's done action
   PlayerCommand::execute_done();
}

bool PlayerSetAudioBufferCommand::store(CommandIOData& data) const{
   PlayerCommand::store(data, "PlayerSetAudioBufferCommand");
   //XXX how to do this one?  maybe associate a global map of files loaded,
   //indicies to file names or database indicies?
   return false;
}

AudioBuffer * PlayerSetAudioBufferCommand::buffer() const {
   return mBuffer;
}

void PlayerSetAudioBufferCommand::buffer(AudioBuffer * buffer) {
   mBuffer = buffer;
}

PlayerSetBeatBufferCommand::PlayerSetBeatBufferCommand(unsigned int idx, 
      BeatBuffer * buffer,
      bool deleteOldBuffer) : 
   PlayerCommand(idx), 
   mBuffer(buffer),
   mOldBuffer(NULL),
   mDeleteOldBuffer(deleteOldBuffer)
{ }

PlayerSetBeatBufferCommand::~PlayerSetBeatBufferCommand() {
   if (mOldBuffer && mDeleteOldBuffer) {
      delete mOldBuffer;
      mOldBuffer = NULL;
   }
}

void PlayerSetBeatBufferCommand::execute(){
   Player * p = player(); 
   if(p != NULL){
      //store the time executed
      position_executed(p->position());
      //store the old buffer pointer
      mOldBuffer = p->beat_buffer();
      //execute the action
      p->beat_buffer(mBuffer);
   }
}

void PlayerSetBeatBufferCommand::execute_done(){
   if (mOldBuffer && mDeleteOldBuffer) {
      delete mOldBuffer;
      mOldBuffer = NULL;
   }
   //execute our super class's done action
   PlayerCommand::execute_done();
}

bool PlayerSetBeatBufferCommand::store(CommandIOData& data) const{
   PlayerCommand::store(data, "PlayerSetBeatBufferCommand");
   //XXX how to do this one?  maybe associate a global map of files loaded,
   //indicies to file names or database indicies?
   return false;
}

BeatBuffer * PlayerSetBeatBufferCommand::buffer() const {
   return mBuffer;
}

void PlayerSetBeatBufferCommand::buffer(BeatBuffer * buffer) {
   mBuffer = buffer;
}

PlayerPositionCommand::PlayerPositionCommand(unsigned int idx, 
      position_t target, const TimePoint & timepoint) : 
   PlayerCommand(idx),
   mTimePoint(timepoint),
   mTarget(target),
   mFrames(0),
   mUseFrames(false)
{ }

PlayerPositionCommand::PlayerPositionCommand(unsigned int idx, 
      position_t target, long frames) : 
   PlayerCommand(idx),
   mTimePoint(),
   mTarget(target),
   mFrames(frames),
   mUseFrames(true)
{ }

void PlayerPositionCommand::execute(){
   Player * p = player(); 
   if(p != NULL){
      //store the time executed
      position_executed(p->position());
      //execute the action
      switch(mTarget){
         case PLAY:
            if (mUseFrames)
               p->position_at_frame(mFrames);
            else
               p->position(mTimePoint);
            break;
         case PLAY_RELATIVE:
            if (mUseFrames)
               p->position_at_frame_relative(mFrames);
            else
               p->position_relative(mTimePoint);
            break;
         case START:
            //TODO mUseFrames
            p->start_position(mTimePoint);
            break;
         case END:
            p->end_position(mTimePoint);
            break;
         case LOOP_START:
            p->loop_start_position(mTimePoint);
            break;
         case LOOP_END:
            p->loop_end_position(mTimePoint);
            break;
      };
      //TODO shouldn't it update the position if dirty?
   }
}

bool PlayerPositionCommand::store(CommandIOData& data) const{
   PlayerCommand::store(data, "PlayerPositionCommand");
   switch(mTarget){
      case PLAY:
         data["target"] = "play";
         break;
      case PLAY_RELATIVE:
         data["target"] = "play_relative";
         break;
      case START:
         data["target"] = "start";
         break;
      case END:
         data["target"] = "end";
         break;
      case LOOP_START:
         data["target"] = "loop_start";
         break;
      case LOOP_END:
         data["target"] = "loop_end";
         break;
   };
   //XXX string representation of time point;
   //data["time_point"] = mTimePoint;
   return false;
}

