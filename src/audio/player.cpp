#include "player.hpp"
#include <cmath>
#include "master.hpp"
#include "stretcher.hpp"
#include "stretcherrate.hpp"
#include "defines.hpp"
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define DJ_EQ_URI "http://plugin.org.uk/swh-plugins/dj_eq"

#include <iostream>
#include <iomanip>
using std::cerr;
using std::endl;

using namespace dj::audio;

Player::Player() : 
  mBeatIndex(0),
  mLoopStartFrame(0),
  mLoopEndFrame(0),
  mMaxSampleValue(0.0),
  mEnvelope(dj::audio::quarter_sin, 4410),
  mFadeoutIndex(0)
#ifdef USE_LV2
  ,mEqInstance(NULL)
#endif
{
  //states
  mPlayState = PAUSE;
  mOutState = CUE;

  //TODO make this configurable
  mCueMutesMain = false;
  mMute = false;
  mSync = false;
  mLoop = false;

  //continuous
  mVolume = 1.0;

  mVolumeBuffer = NULL;
  mBeatBuffer = NULL;

  //XXX who should keep track of these stretchers?
  mStretcher = new StretcherRate();

  mSetup = false;
}

Player::~Player(){
  //cleanup
  if(mVolumeBuffer)
    delete [] mVolumeBuffer;
#ifdef USE_LV2
  if(mEqInstance)
    lilv_instance_free(mEqInstance);
#endif
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

  unsigned int fade_length = static_cast<double>(sampleRate) * 0.015; //15 ms
  mEnvelope.length(fade_length);
  mFadeoutBuffer.resize(2 * (fade_length + 1));
  mFadeoutIndex = mFadeoutBuffer.size();

#ifdef USE_LV2
  //do lv2
  Master * master = Master::instance();
  LilvNode * plugin_uri = lilv_new_uri(master->lv2_world(), DJ_EQ_URI);
  const LilvPlugin * eq_plugin = lilv_plugins_get_by_uri(master->lv2_plugins(), plugin_uri);
  if (!eq_plugin) {
    cerr << "could not load eq lv2 plugin, do you have dj eq installed?:" << endl;
    cerr << "\t\t" << DJ_EQ_URI << endl;
  } else {
    //load the plugin
    mEqInstance = lilv_plugin_instantiate(eq_plugin, mSampleRate, NULL);
    if (!mEqInstance) {
      cerr << "could not instantiate eq lv2 plugin, do you have dj eq installed?:" << endl;
      cerr << "\t\t" << DJ_EQ_URI << endl;
    } else {
      lilv_instance_connect_port(mEqInstance, 0, &mEqControl.low);
      lilv_instance_connect_port(mEqInstance, 1, &mEqControl.mid);
      lilv_instance_connect_port(mEqInstance, 2, &mEqControl.high);
      lilv_instance_connect_port(mEqInstance, 7, &mEqControl.latency);
      lilv_instance_activate(mEqInstance);
    }
  }
#endif

  mSetup = true;
}

//the audio computation methods
//setup for audio computation
void Player::audio_pre_compute(unsigned int /* numFrames */, float ** /* mixBuffer */, const Transport& /* transport */){ 
  if(!mStretcher->audio_buffer())
    return;
  if(mStretcher->frame() >= mStretcher->audio_buffer()->length())
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
  if(mPlayState == PLAY) {
    //only update the rate on the beat.
    if(inbeat && mSync && mBeatBuffer) {
      mBeatIndex = mBeatBuffer->index(mStretcher->seconds());
      update_play_speed(&transport);
    }

    float buffer[2];
    mStretcher->next_frame(buffer);

    if (!mEnvelope.at_end()) {
      double v = mEnvelope.value_step();
      buffer[0] *= v;
      buffer[1] *= v;
    }

    mixBuffer[0][frame] = buffer[0];
    mixBuffer[1][frame] = buffer[1];

    if (mLoop) {
      if(mLoopEndFrame > mLoopStartFrame && mStretcher->frame() >= mLoopEndFrame)
        position_at_frame(mLoopStartFrame);
    }
  }

  if (mFadeoutIndex < mFadeoutBuffer.size()) {
    mixBuffer[0][frame] += mFadeoutBuffer[mFadeoutIndex];
    mixBuffer[1][frame] += mFadeoutBuffer[mFadeoutIndex + 1];
    mFadeoutIndex += 2;
  }
}

//finalize audio computation, apply effects, etc.
void Player::audio_post_compute(unsigned int numFrames, float ** mixBuffer){
  if(!mStretcher->audio_buffer())
    return;
#ifdef USE_LV2
  if(mEqInstance) {
    lilv_instance_connect_port(mEqInstance, 3, mixBuffer[0]);
    lilv_instance_connect_port(mEqInstance, 4, mixBuffer[1]);
    lilv_instance_connect_port(mEqInstance, 5, mixBuffer[0]);
    lilv_instance_connect_port(mEqInstance, 6, mixBuffer[1]);
    lilv_instance_run(mEqInstance, numFrames);
  }
#endif
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
        float sample_with_volume = mixBuffer[i][j] * mVolumeBuffer[j];

        cueBuffer[i][j] = mixBuffer[i][j];
        mMaxSampleValue = std::max(mMaxSampleValue, fabsf(sample_with_volume));

        if (mCueMutesMain)
          mixBuffer[i][j] = 0.0f;
        else
          mixBuffer[i][j] = sample_with_volume;
      }
    }
  } else {
    for(unsigned int i = 0; i < 2; i++){
      for(unsigned int j = 0; j < numFrames; j++) {
        cueBuffer[i][j] = 0.0f;
        mixBuffer[i][j] *= mVolumeBuffer[j];
        mMaxSampleValue = std::max(mMaxSampleValue, fabsf(mixBuffer[i][j]));
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

unsigned int Player::frame() const { return (!mStretcher->audio_buffer()) ? 0 : mStretcher->frame(); }
unsigned int Player::beat_index() const { return mBeatIndex; }
float Player::max_sample_value() const { return mMaxSampleValue; }

double Player::bpm() {
  if (!mBeatBuffer)
    return 0.0;

  mBeatIndex = mBeatBuffer->index(mStretcher->seconds());
  unsigned int beat = mBeatIndex;
  if (beat + 1 >= mBeatBuffer->length())
    beat = mBeatBuffer->length() - 2;

  const double dist = (mBeatBuffer->at(beat + 1) - mBeatBuffer->at(beat));
  const double s_per_beat = dist / play_speed();

  return 60.0 / s_per_beat; //becomes beats per min
}

double Player::pos_in_beat() const {
  if (!mBeatBuffer || !mStretcher->audio_buffer())
    return 0.0;

  double seconds = mStretcher->seconds();
  unsigned int beat = mBeatBuffer->index(seconds);

  return pos_in_beat(seconds, beat);
}

bool Player::audible() const {
  if (!mStretcher->audio_buffer() || muted() || play_state() == PAUSE || volume() < INAUDIBLE_VOLUME || frame() >= mStretcher->audio_buffer()->length())
    return false;
  return true;
}

AudioBuffer * Player::audio_buffer() const { return mStretcher->audio_buffer(); }
BeatBuffer * Player::beat_buffer() const { return mBeatBuffer; }

//setters
void Player::play_state(play_state_t val, Transport * transport) {
  if (mPlayState != val) {
    mPlayState = val;

    if (mBeatBuffer) {
      //sync to transport if we should
      if (transport && mSync && mPlayState == PLAY)
        sync_to_transport(transport);
      else
        mBeatIndex = mBeatBuffer->index(mStretcher->seconds());
    }

    //render our fadeout
    if (mPlayState == PAUSE && audio_buffer()) {
      const unsigned int frame = mStretcher->frame();
      fill_fade_buffer();
      mStretcher->frame(frame);
      mFadeoutIndex = 0;
    } else if (mPlayState == PLAY) {
      mEnvelope.reset();
    }
  }
}

void Player::out_state(out_state_t val){
  mOutState = val;
}

void Player::mute(bool val){
  mMute = val;
}

void Player::sync(bool val, const Transport * transport) {
  if (val != mSync) {
    mSync = val;
    if (transport && mSync && mPlayState == PLAY)
      sync_to_transport(transport);
  }
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

void Player::position_at_frame(unsigned long frame, Transport * transport) {
  if (!mStretcher->audio_buffer())
    return;

  if (mPlayState == PLAY)
    setup_seek_fade();
  mStretcher->frame(frame);

  if (mBeatBuffer) {
    if (transport && mSync && mPlayState == PLAY)
      sync_to_transport(transport);
    else
      mBeatIndex = mBeatBuffer->index(mStretcher->seconds());
  }
}

void Player::position_at_beat(unsigned int beat, Transport * transport) {
  if (!mStretcher->audio_buffer() || !mBeatBuffer || mBeatBuffer->length() <= beat)
    return;

  if (mPlayState == PLAY)
    setup_seek_fade();
  mBeatIndex = beat;

  //find the frame for this beat
  unsigned long frame = static_cast<unsigned long>(mBeatBuffer->at(beat) * mStretcher->audio_buffer()->sample_rate());
  position_at_frame(frame, transport);
}

void Player::position_at_beat_relative(int offset, Transport * transport) {
  if (!mStretcher->audio_buffer() || !mBeatBuffer)
    return;

  //find our current position
  const double current_seconds = mStretcher->seconds();
  const unsigned int current_beat = mBeatBuffer->index(current_seconds);
  unsigned long frame = 0;

  if (offset < 0 && current_beat < static_cast<unsigned int>(-offset)) {
    mBeatIndex = 0;
  } else {
    //offset the beat
    unsigned int new_beat = current_beat + offset;

    if (new_beat + 1 < mBeatBuffer->length()) {
      //find the new position in seconds, offset by our old beat offset
      double new_seconds = mBeatBuffer->at(new_beat);
      new_seconds += pos_in_beat(current_seconds, current_beat) * (mBeatBuffer->at(new_beat + 1) - new_seconds);

      frame = new_seconds * mStretcher->audio_buffer()->sample_rate();
    } else {
      new_beat = mBeatBuffer->length() - 1;
      frame = mBeatBuffer->at(mBeatBuffer->length() - 1) * mStretcher->audio_buffer()->sample_rate();
    }
    mBeatIndex = new_beat;
  }
  position_at_frame(frame, transport);
}

void Player::loop_start_frame(unsigned int val){
  mLoopStartFrame = val;
}

void Player::loop_end_frame(unsigned int val){
  mLoopEndFrame = val;
}

void Player::audio_buffer(AudioBuffer * buf){
  mStretcher->audio_buffer(buf);
  mStretcher->frame(0);
}

void Player::beat_buffer(BeatBuffer * buf){
  mBeatBuffer = buf;
  mBeatIndex = 0;
  mStretcher->frame(0);
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

void Player::max_sample_value_reset() { mMaxSampleValue = 0.0; };

void Player::position_at_frame_relative(long offset){
  if (offset < 0 && -offset > mStretcher->frame())
    position_at_frame(0);
  else
    position_at_frame(offset + mStretcher->frame());
}

void Player::play_speed_relative(double amt){
  mStretcher->speed(mStretcher->speed() + amt);
}

void Player::volume_relative(double amt){
  mVolume += amt;
}


void Player::update_play_speed(const Transport * transport) {
  if (!mBeatBuffer || mBeatBuffer->length() < 3)
    return;

  double seconds = mStretcher->seconds();
  unsigned int beat_closest = mBeatBuffer->index_closest(seconds);

  if (beat_closest + 2 >= mBeatBuffer->length()) {
    beat_closest = mBeatBuffer->length() - 3;
    seconds = mBeatBuffer->at(beat_closest);
  }

  //target the next beat
  double target_offset = mBeatBuffer->at(beat_closest + 1) - seconds;
  double time_till_target = transport->seconds_per_beat() * (1.0 - transport->position().pos_in_beat());
  
  if (time_till_target <= 0)
    return;

  double speed = target_offset / time_till_target;
  mStretcher->speed(speed);
}

void Player::sync_to_transport(const Transport * transport) {
  TimePoint trans_pos = transport->position();

  unsigned int beat = mBeatBuffer->index_closest(mStretcher->seconds());
  if (trans_pos.pos_in_beat() > 0.5 && beat > 1)
    beat -= 1;

  if (beat + 1 < mBeatBuffer->length()) {
    //store index
    mBeatIndex = beat;

    //update position
    const double beat_seconds = mBeatBuffer->at(beat);
    const double next_beat_seconds = mBeatBuffer->at(beat + 1);
    const double pos_seconds = beat_seconds + trans_pos.pos_in_beat() * (next_beat_seconds - beat_seconds);
    mStretcher->seconds(pos_seconds);

    //update rate
    double speed = (next_beat_seconds - beat_seconds) / transport->seconds_per_beat();
    mStretcher->speed(speed);
  }

}

double Player::pos_in_beat(double seconds, unsigned int beat) const {
  if (beat + 1 >= mBeatBuffer->length())
    return 0.0;

  double beat_times[2];
  beat_times[0] = mBeatBuffer->at(beat);
  beat_times[1] = mBeatBuffer->at(beat + 1);

  double div = (beat_times[1] - beat_times[0]);
  if (div <= 0.0)
    return 0.0;

  return (seconds - beat_times[0]) / div;
}


void Player::fill_fade_buffer() {
  const unsigned int channels = audio_buffer()->channels();
  const unsigned int fade_frames = mFadeoutBuffer.size() / channels;

  //get our data
  mStretcher->next(&mFadeoutBuffer.front(), fade_frames);

  //apply envelope
  for (unsigned int i = 0; i < fade_frames; i++) {
    double e = mEnvelope.reversed_value_at(i);
    for (unsigned int c = 0; c < channels; c++)
      mFadeoutBuffer[i * channels + c] *= e;
  }
}

void Player::setup_seek_fade() {
  if (mFadeoutIndex < mFadeoutBuffer.size())
    return;

  //fade on seek
  fill_fade_buffer();
  mFadeoutIndex = 0;
  mEnvelope.reset();
}

//command stuff
//
PlayerCommand::PlayerCommand(unsigned int idx) : 
  mMaster(Master::instance())
{
  mIndex = idx;
}

unsigned int PlayerCommand::index() const { return mIndex; }
unsigned int PlayerCommand::beat_executed() const { return mBeatExecuted; }
void PlayerCommand::beat_executed(unsigned int beat){
  mBeatExecuted = beat;
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
    //position_executed(p->position());
    //execute the action
    switch(mAction){
      case PLAY:
        p->play_state(Player::PLAY, master()->transport());
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
        p->sync(true, master()->transport());
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
    //position_executed(p->position());
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
    //position_executed(p->position());
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
    //position_executed(p->position());
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

PlayerPositionCommand::PlayerPositionCommand(unsigned int idx, position_t target, long value) : 
  PlayerCommand(idx),
  mTarget(target),
  mValue(value)
{ }

void PlayerPositionCommand::execute(){
  Player * p = player(); 
  if(p != NULL){
    //store the time executed
    //position_executed(p->position());
    //execute the action
    switch(mTarget){
      case PLAY:
        p->position_at_frame(mValue, master()->transport());
        break;
      case PLAY_RELATIVE:
        p->position_at_frame_relative(mValue);
        break;
      case PLAY_BEAT:
        p->position_at_beat(mValue, master()->transport());
        break;
      case PLAY_BEAT_RELATIVE:
        p->position_at_beat_relative(mValue);
        break;
      case LOOP_START:
        p->loop_start_frame(mValue);
        break;
      case LOOP_END:
        p->loop_end_frame(mValue);
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
    case PLAY_BEAT:
      data["target"] = "play_beat";
      break;
    case PLAY_BEAT_RELATIVE:
      data["target"] = "play_beat_relative";
      break;
    case LOOP_START:
      data["target"] = "loop_start";
      break;
    case LOOP_END:
      data["target"] = "loop_end";
      break;
  };

  data["value"] = mValue;
  return false;
}


PlayerLoopCommand::PlayerLoopCommand(unsigned int idx, long beats, bool start_looping) :
  PlayerCommand(idx),
  mStartLooping(start_looping),
  mBeats(beats)
{
}

PlayerLoopCommand::PlayerLoopCommand(unsigned int idx, long start_frame, long end_frame, bool start_looping) :
  PlayerCommand(idx),
  mStartLooping(start_looping),
  mStartFrame(start_frame), mEndFrame(end_frame)
{
}

void PlayerLoopCommand::execute() {
  Player * p = player();
  BeatBuffer * beat_buff = p->beat_buffer();
  AudioBuffer * audio = p->audio_buffer();
  if (!audio)
    return;
  double sample_rate = static_cast<double>(audio->sample_rate());

  if (mEndFrame < 0) {
    if (!beat_buff)
      return;
    if (mStartFrame < 0) {
      //find both start and end frame based on current location
      //we don't use the closest index, we use the last index before our frame so we stay in the current beat
      unsigned int beat = beat_buff->index(static_cast<double>(p->frame()) / sample_rate);
      if (beat + mBeats >= beat_buff->length())
        return;

      mStartFrame = static_cast<long>(sample_rate * beat_buff->at(beat));
      mEndFrame = static_cast<long>(sample_rate * beat_buff->at(beat + mBeats));
    } else {
      unsigned int beat_end = beat_buff->index_closest(static_cast<double>(mStartFrame) / sample_rate) + mBeats;
      mEndFrame = static_cast<long>(sample_rate * beat_buff->at(beat_end));
    }
  } else if (mStartFrame < 0) {
    if (!beat_buff)
      return;
    unsigned int beat_end = beat_buff->index_closest(static_cast<double>(mEndFrame) / sample_rate);
    if (beat_end < mBeats)
      mStartFrame = 0;
    else
      mStartFrame = static_cast<long>(sample_rate * beat_buff->at(beat_end - mBeats));
  }

  p->loop_start_frame(mStartFrame);
  p->loop_end_frame(mEndFrame);
  if (mStartLooping)
    p->loop(true);
  mLooping = p->looping();
}

bool PlayerLoopCommand::store(CommandIOData& data) const {
  PlayerCommand::store(data, "PlayerLoopCommand");
  data["beats"] = mBeats;
  data["start_frame"] = mStartFrame;
  data["end_frame"] = mEndFrame;
  return false;
}


PlayerLoopShiftCommand::PlayerLoopShiftCommand(unsigned int idx, int beats) :
  PlayerCommand(idx),
  mBeats(beats)
{
}

void PlayerLoopShiftCommand::execute() {
  Player * p = player();
  BeatBuffer * beat_buff = p->beat_buffer();
  AudioBuffer * audio = p->audio_buffer();
  if (!audio || !beat_buff)
    return;
  double sample_rate = static_cast<double>(audio->sample_rate());

  int beat_start = beat_buff->index_closest(static_cast<double>(p->loop_start_frame()) / sample_rate);
  int beat_end = beat_buff->index_closest(static_cast<double>(p->loop_end_frame()) / sample_rate);

  beat_start += mBeats;
  beat_end += mBeats;
  if (beat_start < 0)
    beat_start = 0;
  if (beat_end < 0)
    beat_start = 1;

  mStartFrame = sample_rate * beat_buff->at(beat_start);
  mEndFrame = sample_rate * beat_buff->at(beat_end);

  p->loop_start_frame(mStartFrame);
  p->loop_end_frame(mEndFrame);
  mLooping = p->looping();

  //see if we need to update the position
  if (mLooping) {
    if (mStartFrame > p->frame()) {
      //XXX do anything?
    } else if (mEndFrame <= p->frame()) {
      unsigned int new_frame = mStartFrame + (p->frame() - mEndFrame);
      p->position_at_frame(new_frame);
    }
  }
}

bool PlayerLoopShiftCommand::store(CommandIOData& data) const {
  PlayerCommand::store(data, "PlayerLoopShiftCommand");
  data["beats"] = mBeats;
  data["start_frame"] = mStartFrame;
  data["end_frame"] = mEndFrame;
  return false;
}
