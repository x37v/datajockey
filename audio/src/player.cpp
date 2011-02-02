#include "player.hpp"
#include <math.h>
#include "master.hpp"
#define RUBBERBAND_WINDOW_SIZE 64
#define MIN(x,y) ((x) < (y) ? (x) : (y))

using namespace DataJockey::Audio;

Player::Player() : 
   mPosition(0.0),
   mTransportOffset(0,0)
{
   //states
   mPlayState = PAUSE;
   mOutState = CUE;
   mStretchMethod = PLAY_RATE;
   mMute = false;
   mSync = false;
   mLoop = false;

   //continuous
   mVolume = 1.0;
   mPlaySpeed = 1.0;

   mVolumeBuffer = NULL;
   mAudioBuffer = NULL;
   mBeatBuffer = NULL;
   mSampleIndex = 0;
   mSampleIndexResidual = 0.0;
   mRubberBandStretcher = NULL;

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
   if(mRubberBandStretcher)
      delete mRubberBandStretcher;
}

//this creates internal buffers
//** must be called BEFORE the audio callback starts
void Player::setup_audio(
      unsigned int sampleRate,
      unsigned int maxBufferLen){
   //set the sample rate, create our internal audio buffers
   mSampleRate = sampleRate;
   if(mVolumeBuffer)
      delete [] mVolumeBuffer;
   if(mRubberBandStretcher)
      delete mRubberBandStretcher;
   mVolumeBuffer = new float[maxBufferLen];
   mRubberBandStretcher = new 
      RubberBand::RubberBandStretcher(mSampleRate, 2,
            RubberBand::RubberBandStretcher::OptionProcessRealTime | 
            RubberBand::RubberBandStretcher::OptionThreadingNever);
   //XXX what is the ideal size?
   mRubberBandStretcher->setMaxProcessSize(maxBufferLen * 4);
   mSetup = true;
}

//the audio computation methods
//setup for audio computation
void Player::audio_pre_compute(unsigned int numFrames, float ** mixBuffer,
      const Transport& transport){ 
   if(!mAudioBuffer)
      return;

   //do we need to update the mSampleIndex based on the position?
   if(mPositionDirty)
      update_position(transport);

   if(mUpdateTransportOffset) {
      update_transport_offset(transport);
      if (mBeatBuffer && mSync)
         update_play_speed(transport);
   }

   if(mSampleIndex + mSampleIndexResidual >= mAudioBuffer->length())
      return;
   if(mEndPosition.valid() && mPosition >= mEndPosition)
      return;

   if(mStretchMethod == RUBBER_BAND){
      if(mPlayState == PLAY){
         //XXX SYNC not implemented yet!
         mRubberBandStretcher->setTimeRatio(1.0 / mPlaySpeed);
         while(mRubberBandStretcher->available() < (int)numFrames){
            unsigned int winSize = MIN(RUBBERBAND_WINDOW_SIZE, numFrames);
            for(unsigned int i = 0; i < 2; i++){
               for(unsigned int j = 0; j < winSize; j++){
                  mixBuffer[i][j] = mAudioBuffer->sample(i, mSampleIndex + j);
               }
            }
            mSampleIndex += winSize;
            mRubberBandStretcher->process(mixBuffer, winSize, false);
         }
         mSampleIndexResidual = 0;
         //update our position
         if(mBeatBuffer){
            mPosition = mBeatBuffer->position_at_time(
                  (double)mSampleIndex / (double)mSampleRate, mPosition);
         }
      }
   } 
}

//actually compute one frame, filling an internal buffer
//syncing to the transport if mSync == true
void Player::audio_compute_frame(unsigned int frame, float ** mixBuffer, 
      const Transport& transport, bool inbeat){
   //zero out the frame;
   mixBuffer[0][frame] = mixBuffer[1][frame] = 0.0;

   if(!mAudioBuffer)
      return;
   //compute the volume
   mVolumeBuffer[frame] = mMute ? 0.0 : mVolume;

   //compute the actual frame
   if(mPlayState == PLAY){
      switch(mStretchMethod){
         case PLAY_RATE:

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

            for(unsigned int i = 0; i < 2; i++){
               mixBuffer[i][frame] = 
                  mAudioBuffer->sample(i, mSampleIndex, mSampleIndexResidual);
            }

            //don't go past the length
            if (mSampleIndex < mAudioBuffer->length()) {
               mSampleIndexResidual += mPlaySpeed;
               mSampleIndex += floor(mSampleIndexResidual);
               mSampleIndexResidual -= floor(mSampleIndexResidual);
            }

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
            break;
         case RUBBER_BAND:
            float *tmp[2], vals[2];
            tmp[0] = &vals[0];
            tmp[1] = &vals[2];

            if(mRubberBandStretcher->retrieve(tmp, 1)){
               for(unsigned int i = 0; i < 2; i++){
                  mixBuffer[i][frame] = *tmp[i];
               }
            }
            break;
         default:
            break;
      }
   }
}

//finalize audio computation, apply effects, etc.
   void Player::audio_post_compute(unsigned int /*numFrames*/, float ** /*mixBuffer*/){
      if(!mAudioBuffer)
         return;
   }

//actually fill the output vectors
void Player::audio_fill_output_buffers(unsigned int numFrames,
      float ** mixBuffer, float ** cueBuffer){
   if(!mAudioBuffer)
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
         for(unsigned int j = 0; j < numFrames; j++)
            mixBuffer[i][j] *= mVolumeBuffer[j];
      }
   }
}

//getters
Player::play_state_t Player::play_state() const { return mPlayState; }
Player::out_state_t Player::out_state() const { return mOutState; }
Player::stretch_method_t Player::stretch_method() const { return mStretchMethod; }
bool Player::muted() const { return mMute; }
bool Player::syncing() const { return mSync; }
bool Player::looping() const { return mLoop; }
double Player::volume() const { return mVolume; }
double Player::play_speed() const { return mPlaySpeed; }
const TimePoint& Player::position() const { return mPosition; }
const TimePoint& Player::start_position() const { return mStartPosition; }
const TimePoint& Player::end_position() const { return mEndPosition; }
const TimePoint& Player::loop_start_position() const { return mLoopStartPosition; }
const TimePoint& Player::loop_end_position() const { return mLoopEndPosition; }
unsigned long Player::current_frame() const { return mSampleIndex; }
AudioBuffer * Player::audio_buffer() const { return mAudioBuffer; }
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

void Player::stretch_method(stretch_method_t /*val*/){
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
   mPlaySpeed = val;
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
      mSampleIndex = (double)mSampleRate * mPosition.seconds();
      mSampleIndexResidual = 0;
   } else if(mBeatBuffer){
      mSampleIndex = mSampleRate * mBeatBuffer->time_at_position(mPosition);
      mSampleIndexResidual = 0;
   } else
      mPositionDirty = true;

}

void Player::position_at_frame(unsigned long frame) {
   mSampleIndex = frame;
   mSampleIndexResidual = 0;
   mUpdateTransportOffset = true;

   if (mAudioBuffer && mAudioBuffer->length() <= mSampleIndex) {
      mSampleIndex = mAudioBuffer->length();
   }

   if (mBeatBuffer) {
      double time = mSampleIndex;
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
   mAudioBuffer = buf;
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


//misc
void Player::position_relative(TimePoint amt){
   position(mPosition + amt);
}

void Player::position_at_frame_relative(long offset){
   long frame = mSampleIndex + offset;
   if (frame > 0)
      position_at_frame(frame);
   else
      position_at_frame(0);
}

void Player::play_speed_relative(double amt){
   mPlaySpeed += amt;
}

void Player::volume_relative(double amt){
   mVolume += amt;
}


void Player::update_position(const Transport& transport){
   mPositionDirty = false;

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
}

void Player::update_play_speed(const Transport& transport) {
   double secTillBeat = transport.seconds_till_next_beat();
   if (secTillBeat <= 0)
      return;

   TimePoint next = mPosition;
   next.advance_beat();

   double newSpeed = mBeatBuffer->time_at_position(next) - 
      mBeatBuffer->time_at_position(mPosition);
   newSpeed /= secTillBeat; 
   //XXX should make this a setting
   if(newSpeed > 0.25 && newSpeed < 4)
      mPlaySpeed = newSpeed;
}

void Player::update_transport_offset(const Transport& transport) {
   mTransportOffset = (transport.position() - mPosition);
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
            p->volume(mValue);
            break;
         case VOLUME_RELATIVE:
            p->volume_relative(mValue);
            break;
         case PLAY_SPEED:
            p->play_speed(mValue);
            break;
         case PLAY_SPEED_RELATIVE:
            p->play_speed_relative(mValue);
            break;
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

