#include "player.hpp"
#include <cmath>
#include "master.hpp"
#include "stretcher.hpp"
#include "stretcherrate.hpp"
#include "defines.hpp"
#include "config.hpp"
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#include <algorithm>
#include <iostream>
#include <iomanip>
using std::cout;
using std::cerr;
using std::endl;

using namespace djaudio;

namespace {

  int beat_index(BeatBuffer * beat_buffer, int frame) {
    //finds the first element which is greater than frame
    auto it = std::upper_bound(beat_buffer->begin(), beat_buffer->end(), frame);
    if (it == beat_buffer->end())
      return beat_buffer->size() - 1;
    if (it == beat_buffer->begin())
      return 0;
    return it - beat_buffer->begin() - 1;
  }

  int closest_index(BeatBuffer * beat_buffer, int frame) {
    int beat = beat_index(beat_buffer, frame);
    if (beat + 1 == static_cast<int>(beat_buffer->size()))
      return beat;
    int diffs[2] = {frame - beat_buffer->at(beat), beat_buffer->at(beat + 1) - frame};
    if (diffs[0] < diffs[1])
      return beat;
    return beat + 1;
  }
}

Player::Player() : 
  mBeatIndex(0),
  mLoopStartFrame(0),
  mLoopEndFrame(0),
  mMaxSampleValue(0.0),
  mEnvelope(djaudio::quarter_sin, 4410),
  mBumpEnvelope(djaudio::ramp_down, 44100 * 10),
  mFadeoutIndex(0)
#ifdef USE_LV2
  ,mEqPlugin(NULL)
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
  for (unsigned int i = 0; i < mSendVolumeBuffers.size(); i++)
    delete [] mSendVolumeBuffers[i];
  mSendVolumeBuffers.clear();
#ifdef USE_LV2
  if(mEqPlugin)
    delete mEqPlugin;
#endif
}

//this creates internal buffers
//** must be called BEFORE the audio callback starts
void Player::setup_audio(
    unsigned int sampleRate,
    unsigned int maxBufferLen,
    unsigned int sendBufferCount) {
  //set the sample rate, create our internal audio buffers
  mSampleRate = sampleRate;
  mBumpEnvelope.length(mSampleRate * 10);
  if(mVolumeBuffer)
    delete [] mVolumeBuffer;
  mVolumeBuffer = new float[maxBufferLen];
  for (unsigned int i = 0; i < mSendVolumeBuffers.size(); i++)
    delete [] mSendVolumeBuffers[i];
  mSendVolumeBuffers.clear();
  mSendVolumes.clear();

  unsigned int fade_length = static_cast<double>(sampleRate) * 0.020; //seconds
  mEnvelope.length(fade_length);
  mFadeoutBuffer.resize(2 * fade_length);
  mFadeoutIndex = mFadeoutBuffer.size();

  for (unsigned int i = 0; i < sendBufferCount; i++) {
    float * v = new float[maxBufferLen];
    memset(v, 0, sizeof(float) * maxBufferLen);
    mSendVolumeBuffers.push_back(v);
    mSendVolumes.push_back(0.0f);
  }

#ifdef USE_LV2
  dj::Configuration * config = dj::Configuration::instance();
  try {
    mEqPlugin = new Lv2Plugin(config->eq_uri());
    mEqPlugin->setup(sampleRate, maxBufferLen);
    for (int i = 0; i < 3; i++) {
      dj::eq_band_t band = static_cast<dj::eq_band_t>(i);
      mEqBandPortMapping[i] = mEqPlugin->port_index(config->eq_port_symbol(band));
      mEqBandValueMax[i] = mEqPlugin->port_value_max(mEqBandPortMapping[i]);
      mEqBandValueMin[i] = mEqPlugin->port_value_min(mEqBandPortMapping[i]);
      mEqBandValueDefault[i] = mEqPlugin->port_value_default(mEqBandPortMapping[i]);
      mEqBandValueDBScale[i] = config->eq_band_db_scale(band);
    }
    QString preset = config->eq_plugin_preset_file();
    if (preset.size())
      mEqPlugin->load_preset_from_file(preset);
  } catch (std::runtime_error& e) {
    if (mEqPlugin) {
      delete mEqPlugin;
      mEqPlugin = nullptr;
    }
    cerr << "error loading plugin: " << e.what() << endl;
    cerr << "do you have it installed?:" << endl;
    cerr << "\t\t" << qPrintable(config->eq_uri()) << endl;
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

  for (unsigned int i = 0; i < mSendVolumes.size(); i++)
    mSendVolumeBuffers[i][frame] = mSendVolumes[i];

  //compute the actual frame
  if(mPlayState == PLAY) {
    //only update the rate on the beat.
    if (inbeat && mSync && mBeatBuffer) {
      mBeatIndex = ::beat_index(mBeatBuffer, mStretcher->frame());
      update_play_speed(&transport);
    }

    float buffer[2];
    if (mBumpState == BUMP_OFF) {
      mStretcher->next_frame(buffer);
    } else {
      double rate_offset = mBumpState == BUMP_REV ? mBumpEnvelope.value() : (1.0 + mBumpEnvelope.reversed_value()) ;
      mBumpEnvelope.step();
      mStretcher->next_frame(buffer, rate_offset);
    }

    if (!mEnvelope.at_end()) {
      double v = mEnvelope.value_step();
      buffer[0] *= v;
      buffer[1] *= v;
    }

    mixBuffer[0][frame] = buffer[0];
    mixBuffer[1][frame] = buffer[1];

  }

  if ((mFadeoutIndex + 1) < mFadeoutBuffer.size()) {
    mixBuffer[0][frame] += mFadeoutBuffer[mFadeoutIndex];
    mixBuffer[1][frame] += mFadeoutBuffer[mFadeoutIndex + 1];
    mFadeoutIndex += 2;
  }

  if (mLoop && mPlayState == PLAY) {
    if(mLoopEndFrame > mLoopStartFrame && mStretcher->frame() >= mLoopEndFrame)
      position_at_frame(mLoopStartFrame);
  }
}

//finalize audio computation, apply effects, etc.
void Player::audio_post_compute(unsigned int numFrames, float ** mixBuffer){
  if(!mStretcher->audio_buffer())
    return;
#ifdef USE_LV2
  if(mEqPlugin)
    mEqPlugin->compute(numFrames, mixBuffer);
#endif
}

//actually fill the output vectors
void Player::audio_fill_output_buffers(unsigned int numFrames,
    float ** mixBuffer, float ** cueBuffer, std::vector<float **>& sendBuffers){
  if(!mStretcher->audio_buffer()) {
    mMaxSampleValue = std::max(mMaxSampleValue, 0.0f);
    return;
  }

  //send the data out, copying to the cue buffer before volume if needed
  const bool cueing = (mOutState == CUE);
  const bool mute_main = cueing && mCueMutesMain;
  for (unsigned int i = 0; i < 2; i++){
    for (unsigned int j = 0; j < numFrames; j++){
      float sample_with_volume = mixBuffer[i][j] * mVolumeBuffer[j];

      cueBuffer[i][j] = cueing ? mixBuffer[i][j] : 0.0f;
      mMaxSampleValue = std::max(mMaxSampleValue, fabsf(sample_with_volume));
      mixBuffer[i][j] = mute_main ? 0.0f : sample_with_volume;
      for (unsigned int k = 0; k < std::min(mSendVolumeBuffers.size(), sendBuffers.size()); k++)
        sendBuffers[k][i][j] += mixBuffer[i][j] * mSendVolumeBuffers[k][j];
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
float Player::send_volume(unsigned int index) const {
  if (index >= mSendVolumes.size())
    return 0.0f;
  return mSendVolumes[index];
}

double Player::play_speed() const {
  return mStretcher->speed() / mSampleRateMult;
}

unsigned int Player::frame() const { return (!mStretcher->audio_buffer()) ? 0 : mStretcher->frame(); }
unsigned int Player::beat_index() const { return mBeatIndex; }
float Player::max_sample_value() const { return mMaxSampleValue; }

double Player::bpm() {
  if (!mBeatBuffer)
    return 0.0;

  mBeatIndex = ::beat_index(mBeatBuffer, mStretcher->frame());
  unsigned int beat = mBeatIndex;
  if (beat + 1 >= mBeatBuffer->size())
    beat = mBeatBuffer->size() - 2;

  const double seconds = static_cast<double>(mBeatBuffer->at(beat + 1) - mBeatBuffer->at(beat)) / static_cast<double>(mSampleRate);
  const double s_per_beat = seconds / mStretcher->speed();

  return 60.0 / s_per_beat; //becomes beats per min
}

double Player::pos_in_beat() const {
  if (!mBeatBuffer || !mStretcher->audio_buffer())
    return 0.0;

  int beat = ::beat_index(mBeatBuffer, mStretcher->frame());
  return pos_in_beat(mStretcher->frame(), beat);
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
        update_play_speed(transport);
      else
        mBeatIndex = ::beat_index(mBeatBuffer, mStretcher->frame());
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
      update_play_speed(transport);
  }
}

void Player::loop(bool val){
  mLoop = val;
}

void Player::bump_start(bool forward) {
  mBumpEnvelope.reset();
  mBumpState = forward ? BUMP_FWD : BUMP_REV;
}

void Player::bump_stop() {
  mBumpState = BUMP_OFF;
}

void Player::volume(double val){
  mVolume = val;
}

void Player::send_volume(unsigned int send_index, float val) {
  if (send_index >= mSendVolumes.size())
    return;
  mSendVolumes[send_index] = val;
}

void Player::play_speed(double val){
  mStretcher->speed(val * mSampleRateMult);
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
      mBeatIndex = ::beat_index(mBeatBuffer, mStretcher->frame());
  }
}

void Player::position_at_beat(unsigned int beat, Transport * transport) {
  if (!mStretcher->audio_buffer() || !mBeatBuffer || mBeatBuffer->size() <= beat)
    return;

  mBeatIndex = beat;
  position_at_frame(mBeatBuffer->at(beat), transport);
}

void Player::position_at_beat_relative(int offset, Transport * transport) {
  if (!mStretcher->audio_buffer() || !mBeatBuffer)
    return;

  //find our current position
  const unsigned int current_beat = ::beat_index(mBeatBuffer, mStretcher->frame());
  unsigned long frame = 0;

  if (offset < 0 && current_beat < static_cast<unsigned int>(-offset)) {
    mBeatIndex = 0;
  } else {
    //offset the beat
    unsigned int new_beat = current_beat + offset;

    if (new_beat + 1 < mBeatBuffer->size()) {
      int beat_frames[2] = { mBeatBuffer->at(new_beat), mBeatBuffer->at(new_beat + 1) };
      int frame_offset = static_cast<int>(
          static_cast<double>(beat_frames[1] - beat_frames[0]) * pos_in_beat(mStretcher->frame(), current_beat));

      frame = beat_frames[0] + frame_offset;
    } else {
      new_beat = mBeatBuffer->size() - 1;
      frame = mBeatBuffer->at(new_beat);
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
  if (buf) {
    double speed_last = play_speed();
    mSampleRateMult = buf->sample_rate() / static_cast<double>(mSampleRate);
    if (!mSync)
      play_speed(speed_last);
  }
  mStretcher->audio_buffer(buf);
  mStretcher->frame(0);
}

void Player::beat_buffer(BeatBuffer * buf){
  mBeatBuffer = buf;
  mBeatIndex = 0;
  mStretcher->frame(0);
}

void Player::eq(dj::eq_band_t band, double value) {
#ifdef USE_LV2
  if (!mEqPlugin)
    return;
  //if we have a negative to positive range, map our -1..1 to that range
  if (mEqBandValueDBScale[band] != 0) {
    value = dj::db2amp(mEqBandValueDBScale[band] * value);
  } else if (mEqBandValueMin[band] < 0) {
    if (value < 0.0)
      value *= -mEqBandValueMin[band];
    else
      value *= mEqBandValueMax[band];
  } else {
    //otherwise map our -1..0..1 to min..default..max
    if (value >= 0.0)
      value = mEqBandValueDefault[band] + value * (mEqBandValueMax[band] - mEqBandValueDefault[band]);
    else 
      value = mEqBandValueDefault[band] + value * (mEqBandValueDefault[band] - mEqBandValueMin[band]);
  }
  value = dj::clamp(value, (double)mEqBandValueMin[band], (double)mEqBandValueMax[band]);
  mEqPlugin->control_value(mEqBandPortMapping[band], value);
#endif
}

float Player::max_sample_value_reset() {
  float v = mMaxSampleValue;
  mMaxSampleValue = 0.0;
  return v;
};

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
  if (!mBeatBuffer || mBeatBuffer->size() < 3)
    return;

  //find the closest index
  int frame = mStretcher->frame();
  int beat_closest = closest_index(mBeatBuffer, frame);

  if (beat_closest + 2 >= static_cast<int>(mBeatBuffer->size())) {
    beat_closest = mBeatBuffer->size() - 3;
    frame = mBeatBuffer->at(beat_closest);
  }

  //target the next beat
  double transport_frames_till_target = transport->frames_per_beat() * (1.0 - transport->position().pos_in_beat());
  
  if (transport_frames_till_target <= 0)
    return;

  //if we're looping we might have a loop that is smaller than a beat
  if (mLoop && frame > 0 &&
      static_cast<unsigned int>(frame) >= mLoopStartFrame &&
      static_cast<unsigned int>(frame) < mLoopEndFrame) {
    int loop_frames = mLoopEndFrame - mLoopStartFrame;
    int loop_beat_start = ::beat_index(mBeatBuffer, frame);
    if (loop_beat_start + 1 < static_cast<int>(mBeatBuffer->size()) && loop_frames > 0) {
      int beat_frames = mBeatBuffer->at(loop_beat_start + 1) - mBeatBuffer->at(loop_beat_start);
      double loop_size = static_cast<double>(loop_frames) / static_cast<double>(beat_frames);
      //adjust frame that we're computing against to wrap at loop size
      if (loop_size < 0.8) {
        //XXX do we always loop from the start of the beat?  if not, we might/should add the subbeat we're starting
        //our loop at
        //currently this just detects if we're closer to the start or the end of the loop
        int closest_sub_beat = round(static_cast<double>(frame - mLoopStartFrame) / static_cast<double>(loop_frames));
        frame -= closest_sub_beat * loop_frames;
        beat_closest = loop_beat_start;
      }
    }
  }

  double frames_till_target = mBeatBuffer->at(beat_closest + 1) - frame;
  double speed = frames_till_target / transport_frames_till_target;

  mStretcher->speed(speed);
}

void Player::sync_to_transport(const Transport * transport) {
  TimePoint trans_pos = transport->position();
  if (!mBeatBuffer)
    return;

  //XXX revisit, should it be closest_index ?
  unsigned int beat = ::beat_index(mBeatBuffer, mStretcher->frame());
  if (trans_pos.pos_in_beat() > 0.5 && beat > 1)
    beat -= 1;

  if (beat + 1 < mBeatBuffer->size()) {
    //store index
    mBeatIndex = beat;

    //update position
    const int beat_frames[2] = {mBeatBuffer->at(beat), mBeatBuffer->at(beat + 1) };
    double frames_in_beat = beat_frames[1] - beat_frames[0];
    int frame = beat_frames[0] + frames_in_beat * trans_pos.pos_in_beat();
    mStretcher->frame(frame);

    //update rate
    double speed = frames_in_beat / static_cast<double>(transport->frames_per_beat());
    mStretcher->speed(speed);
  }
}

double Player::pos_in_beat(int frame, unsigned int beat) const {
  if (beat + 1 >= mBeatBuffer->size())
    return 0.0;

  int beat_times[2] = { mBeatBuffer->at(beat), mBeatBuffer->at(beat + 1) };

  double div = static_cast<double>(beat_times[1] - beat_times[0]);
  if (div <= 0.0)
    return 0.0;

  return static_cast<double>(frame - beat_times[0]) / div;
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

void PlayerStateCommand::execute(const Transport& /*transport*/){
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
      case BUMP_FWD:
        p->bump_start(true);
        break;
      case BUMP_REV:
        p->bump_start(false);
        break;
      case BUMP_OFF:
        p->bump_stop();
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
    case BUMP_FWD:
      data["action"] = "bump_fwd";
      break;
    case BUMP_REV:
      data["action"] = "bump_back";
      break;
    case BUMP_OFF:
      data["action"] = "bump_off";
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

void PlayerDoubleCommand::execute(const Transport& /*transport*/){
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
        p->eq(dj::LOW, mValue); break;
      case EQ_MID:
        p->eq(dj::MID, mValue); break;
      case EQ_HIGH:
        p->eq(dj::HIGH, mValue); break;
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

void PlayerSetAudioBufferCommand::execute(const Transport& /*transport*/){
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

void PlayerSetBeatBufferCommand::execute(const Transport& /*transport*/){
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

PlayerPositionCommand::PlayerPositionCommand(unsigned int idx, position_t target, int value) : 
  PlayerCommand(idx),
  mTarget(target),
  mValue(value)
{ }

void PlayerPositionCommand::execute(const Transport& /*transport*/){
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


PlayerLoopCommand::PlayerLoopCommand(unsigned int idx, double beats, resize_policy_t resize_policy, bool start_looping) :
  PlayerCommand(idx),
  mResizePolicy(resize_policy),
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

void PlayerLoopCommand::execute(const Transport& transport) {
  Player * p = player();
  BeatBuffer * beat_buff = p->beat_buffer();
  AudioBuffer * audio = p->audio_buffer();
  if (!audio)
    return;

  if (p->looping() && mStartFrame < 0 && mEndFrame < 0) {
    switch (mResizePolicy) {
      case RESIZE_FROM_FRONT:
        mStartFrame = p->loop_start_frame();
        break;
      case RESIZE_FROM_BACK:
        mEndFrame = p->loop_end_frame();
        break;
      case RESIZE_AT_POSITION:
      default:
        //do nothing as this is the default when nothing is set
        break;
    }
  }

  int old_loop_frames = 0;
  if (p->looping())
    old_loop_frames = p->loop_end_frame() - p->loop_start_frame();
  const unsigned int beat = beat_buff ? beat_index(beat_buff, p->frame()) : 0;

  if (mEndFrame < 0) {
    if (!beat_buff)
      return;
    unsigned int beat_end = 0;
    if (mStartFrame < 0) {
      //find both start and end frame based on current location
      //we don't use the closest index, we use the last index before our frame so we stay in the current beat
      beat_end = beat + mBeats;
      if (beat_end >= beat_buff->size())
        return;

      mStartFrame = beat_buff->at(beat);
      if (beat == beat_end)
        mEndFrame = mStartFrame;
      else
        mEndFrame = beat_buff->at(beat_end);
    } else {
      beat_end = beat_index(beat_buff, mStartFrame) + mBeats;
      mEndFrame = beat_buff->at(beat_end);
    }

    //deal with fractional part
    double remainder = fmod(mBeats, 1.0);
    if (remainder != 0.0) {
      if (beat_end + 1 < beat_buff->size()) {
        double frames = beat_buff->at(beat_end + 1) - mEndFrame;
        mEndFrame += (frames * remainder);
      } else {
        //XXX what?
      }
    }

  } else if (mStartFrame < 0) {
    if (!beat_buff)
      return;
    unsigned int beat_end = beat_index(beat_buff, mEndFrame);
    if (beat_end < mBeats) {
      mStartFrame = 0;
    } else {
      int beat_start = beat_end - mBeats;
      mStartFrame = beat_buff->at(beat_start);
      if (beat_start >= 1) {
        //deal with fractional part
        double remainder = fmod(mBeats, 1.0);
        if (remainder != 0.0) {
          double frames = mStartFrame - beat_buff->at(beat_start - 1);
          mStartFrame -= (frames * remainder);
          if (mStartFrame < 0)
            mStartFrame = 0;
        }
      }
    }
  }

  if (mStartFrame >= mEndFrame)
    return; //XXX what to do?

  p->loop_start_frame(mStartFrame);
  p->loop_end_frame(mEndFrame);
  if (mStartLooping)
    p->loop(true);
  mLooping = p->looping();

  double loop_frames = static_cast<double>(mEndFrame - mStartFrame);
  //if we are past the end point, fold
  if (p->looping()) {
    if (p->frame() > mEndFrame) {
      int offset = round(loop_frames * floor(static_cast<double>(p->frame() - mStartFrame) / loop_frames));
      p->position_at_frame(p->frame() - offset);
    } else if (old_loop_frames > 0 && loop_frames > old_loop_frames) {
      //XXX untested, do we ever hit this?
      //if the loop grows we might need to offset to before the loop 
      if (p->syncing() && beat_buff && beat + 1 < beat_buff->size()) {
        double beat_frames = beat_buff->at(beat + 1) - beat_buff->at(beat);
        //this is where we should be if we weren't looping
        double target_offset_frames = transport.position().pos_in_beat() * beat_frames;
#if 1
        //XXX hard jump to position we should be at for now
        if (beat_frames > old_loop_frames)
          p->position_at_frame(beat_buff->at(beat) + target_offset_frames);
#else
        int loop_skip_back = target_offset_frames / static_cast<double>(old_loop_frames);
        int offset = loop_skip_back * old_loop_frames;
        p->position_at_frame(p->frame() - offset);
#endif
      }
    }
  }
}

bool PlayerLoopCommand::store(CommandIOData& data) const {
  PlayerCommand::store(data, "PlayerLoopCommand");
  data["beats"] = static_cast<int>(mBeats);
  data["start_frame"] = mStartFrame;
  data["end_frame"] = mEndFrame;
  return false;
}


PlayerLoopShiftCommand::PlayerLoopShiftCommand(unsigned int idx, int beats) :
  PlayerCommand(idx),
  mBeats(beats)
{
}

void PlayerLoopShiftCommand::execute(const Transport& /*transport*/) {
  Player * p = player();
  BeatBuffer * beat_buff = p->beat_buffer();
  AudioBuffer * audio = p->audio_buffer();
  if (!audio || !beat_buff)
    return;

  int beat_start = beat_index(beat_buff, p->loop_start_frame());
  int beat_end = beat_index(beat_buff, p->loop_end_frame());

  beat_start += mBeats;
  beat_end += mBeats;
  if (beat_start < 0 || beat_end <= 0) {
    mStartFrame = p->loop_start_frame();
    mEndFrame = p->loop_end_frame();
    return;
  }

  mStartFrame = beat_buff->at(beat_start);
  mEndFrame = beat_buff->at(beat_end);

  p->loop_start_frame(mStartFrame);
  p->loop_end_frame(mEndFrame);
  mLooping = p->looping();

  //see if we need to update the position
  if (mLooping) {
    if (mStartFrame > p->frame()) {
      //XXX do anything?
    } else if (mEndFrame <= p->frame()) {
      unsigned int new_frame = mStartFrame + (p->frame() - mEndFrame);
      if (new_frame > p->frame())
        new_frame = 0;
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
