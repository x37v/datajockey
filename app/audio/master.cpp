#include "master.hpp"
#include "defines.hpp"
#include <math.h>

using namespace djaudio;

#define DEFAULT_NUM_PLAYERS 2
#define DEFAULT_NUM_SENDS 2
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
  mMaxSampleValue = 0.0f;
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
  for(unsigned int i = 0; i < mSendBuffers.size(); i++){
    delete [] mSendBuffers[i][0];
    delete [] mSendBuffers[i][1];
    delete [] mSendBuffers[i];
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

  if(mPlayerBuffers.size()) {
    for(unsigned int i = 0; i < mPlayerBuffers.size(); i++){
      delete [] mPlayerBuffers[i][0];
      delete [] mPlayerBuffers[i][1];
      delete [] mPlayerBuffers[i];
    }
    mPlayerBuffers.clear();
  }

  if (mSendBuffers.size()) {
    for(unsigned int i = 0; i < mSendBuffers.size(); i++){
      delete [] mSendBuffers[i][0];
      delete [] mSendBuffers[i][1];
      delete [] mSendBuffers[i];
    }
    mSendBuffers.clear();
  }

  mSendPlugins.resize(DEFAULT_NUM_SENDS);
  for (unsigned int i = 0; i < DEFAULT_NUM_SENDS; i++) {
    float ** sampleBuffer = new float*[2];
    sampleBuffer[0] = new float[maxBufferLen];
    sampleBuffer[1] = new float[maxBufferLen];
    memset(sampleBuffer[0], 0, sizeof(float) * maxBufferLen);
    memset(sampleBuffer[1], 0, sizeof(float) * maxBufferLen);
    mSendBuffers.push_back(sampleBuffer);
    mSendPlugins[i].setup(sampleRate, maxBufferLen);
  }

  //set up the players and their buffers
  for(unsigned int i = 0; i < mPlayers.size(); i++){
    float ** sampleBuffer = new float*[2];
    sampleBuffer[0] = new float[maxBufferLen];
    sampleBuffer[1] = new float[maxBufferLen];
    memset(sampleBuffer[0], 0, sizeof(float) * maxBufferLen);
    memset(sampleBuffer[1], 0, sizeof(float) * maxBufferLen);
    mPlayers[i]->setup_audio(sampleRate, maxBufferLen, mSendBuffers.size());
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

#if 0
  //XXX tmp
  
  Lv2Plugin * sendPlugin = new Lv2Plugin(
      "http://calf.sourceforge.net/plugins/VintageDelay");
  sendPlugin->setup(sampleRate, maxBufferLen);
  sendPlugin->load_preset_from_file("/home/alex/lv2presets/delay_lv2.lv2/delay_lv2.ttl");

  AudioPluginPtr shared(sendPlugin);
  AudioPluginNode * node = new AudioPluginNode(shared);
  add_send_plugin(0, node);

  mPlayers[0]->send_volume(0, 1.0);
#endif
}

Player * Master::add_player(){
  Player * p = new Player;
  mPlayers.push_back(p);
  return p;
}

void Master::audio_compute_and_fill(
    JackCpp::AudioIO::audioBufVector outBufferVector,
    unsigned int numFrames) {

  //clear out our sends
  for (unsigned int i = 0; i < mSendBuffers.size(); i++) {
    memset(mSendBuffers[i][0], 0, sizeof(float) * numFrames);
    memset(mSendBuffers[i][1], 0, sizeof(float) * numFrames);
  }

  //execute the schedule
  mScheduler.execute_schedule(mTransport);

  //set up players
  for(unsigned int p = 0; p < mPlayers.size(); p++)
    mPlayers[p]->audio_pre_compute(numFrames, mPlayerBuffers[p], mTransport);

  //compute their samples [and do other stuff]
  for(unsigned int frame = 0; frame < numFrames; frame++){
    //tick the transport
    bool beat = mTransport.tick();
    if (beat) {
      for (unsigned int i = 0; i < mNextBeatCommandBufferIndex; i++) {
        Command * cmd = mNextBeatCommandBuffer[i];
        mNextBeatCommandBuffer[i] = nullptr;
        mScheduler.execute_immediately(cmd, mTransport);
      }
      mNextBeatCommandBufferIndex = 0;
    }
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

  auto compute_player_audio = [this, &outBufferVector](unsigned int player, unsigned int chan, unsigned int frame, float xfade_mul) {
    outBufferVector[chan][frame] += 
      xfade_mul * mPlayerBuffers[player][chan][frame];
    outBufferVector[chan + 2][frame] += mCueVolume * mCueBuffer[chan][frame];
  };

  //finalize each player, and copy its data out
  for(unsigned int p = 0; p < mPlayers.size(); p++){
    mPlayers[p]->audio_post_compute(numFrames, mPlayerBuffers[p]);
    mPlayers[p]->audio_fill_output_buffers(numFrames, mPlayerBuffers[p], mCueBuffer, mSendBuffers);
    int xfade_index = -1;
    if (p == mCrossFadeMixers[0]) {
      xfade_index = 0;
    } else if (p == mCrossFadeMixers[1]) {
      xfade_index = 1;
    }
    for(unsigned int frame = 0; frame < numFrames; frame++) {
      for(unsigned int chan = 0; chan < 2; chan++) {
        float xfade_mul = xfade_index < 0 ? 1.0f : mCrossFadeBuffer[xfade_index][frame];
        compute_player_audio(p, chan, frame, xfade_mul);
      }
    }
  }

  //mix in the effects
  for (unsigned int i = 0; i < mSendPlugins.size(); i++) {
    float ** buf = mSendBuffers[i];
    mSendPlugins[i].compute(numFrames, buf);
    for (unsigned int j = 0; j < numFrames; j++) {
      outBufferVector[0][j] += buf[0][j];
      outBufferVector[1][j] += buf[1][j];
    }
  }

  //XXX do a vector multiply?
  for (unsigned int i = 0; i < numFrames; i++) {
    outBufferVector[0][i] *= mMasterVolumeBuffer[i];
    outBufferVector[1][i] *= mMasterVolumeBuffer[i];
    mMaxSampleValue = std::max(mMaxSampleValue, fabsf(outBufferVector[0][i]));
    mMaxSampleValue = std::max(mMaxSampleValue, fabsf(outBufferVector[1][i]));
  }
}

bool Master::execute_next_beat(Command * cmd) {
  if (mNextBeatCommandBuffer.size() > mNextBeatCommandBufferIndex) {
    mNextBeatCommandBuffer[mNextBeatCommandBufferIndex++] = cmd;
    return true;
  }
  return false;
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

void Master::sync_to_player(unsigned int player_index) {
  if (player_index >= mPlayers.size())
    return;
  Player * player = mPlayers[player_index];
  const double bpm = player->bpm();

  //XXX indicate error?
  if (bpm < 20.0 || bpm > 250.0)
    return;

  const double pos_in_beat = player->pos_in_beat();
  TimePoint position = mTransport.position();
  //move transport forward if the new pos in beat is less than the current
  if (pos_in_beat < position.pos_in_beat())
    position.advance_beat();
  position.pos_in_beat(pos_in_beat);

  //update position and bpm of transport
  mTransport.position(position);
  mTransport.bpm(bpm);

  //update the player, no transport because we're already in sync
  player->sync(true);
}

const std::vector<Player *>& Master::players() const { return mPlayers; }
Scheduler * Master::scheduler(){ return &mScheduler; }
Transport * Master::transport(){ return &mTransport; }

void Master::add_send_plugin(unsigned int send, unsigned int location_index, AudioPluginNode * plugin_node) {
  if (send >= mSendPlugins.size())
    return; //XXX error
  mSendPlugins[send].insert(location_index, plugin_node);
}

float Master::max_sample_value() const { return mMaxSampleValue; }

bool Master::player_audible(unsigned int player_index) const {
  if (player_index >= mPlayers.size() ||
      !mPlayers[player_index]->audible() ||
      master_volume() < INAUDIBLE_VOLUME ||
      (player_index == mCrossFadeMixers[1] && mCrossFadePosition <= INAUDIBLE_VOLUME) ||
      (player_index == mCrossFadeMixers[0] && (1.0 - mCrossFadePosition) <= INAUDIBLE_VOLUME))
    return false;
  return true;
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

float Master::max_sample_value_reset() {
  float v = mMaxSampleValue;
  mMaxSampleValue = 0.0;
  return v;
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

void MasterBoolCommand::execute(const Transport& /*transport*/){
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

void MasterIntCommand::execute(const Transport& /*transport*/) {
  switch(mAction) {
    case SYNC_TO_PLAYER:
      master()->sync_to_player(mValue);
      break;
  }
}

bool MasterIntCommand::store(CommandIOData& /* data */) const {
  //XXX TODO
  return false;
}

MasterDoubleCommand::MasterDoubleCommand(action_t action, double val)
{
  mAction = action;
  mValue = val;
}

void MasterDoubleCommand::execute(const Transport& /*transport*/){
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

void MasterXFadeSelectCommand::execute(const Transport& /*transport*/){
  master()->cross_fade_mixers(mSel[0], mSel[1]);
}

bool MasterXFadeSelectCommand::store(CommandIOData& /*data*/) const {
  //XXX TODO
  return false;
}

MasterNextBeatCommand::MasterNextBeatCommand(Command * command) : mCommand(command) {
}

MasterNextBeatCommand::~MasterNextBeatCommand() {
  if (mCommand)
    delete mCommand;
}

void MasterNextBeatCommand::execute(const Transport& /*transport*/) {
  //XXX use transport to see if we're on the beat already?
  if (master()->execute_next_beat(mCommand))
    mCommand = nullptr;
}

bool MasterNextBeatCommand::store(CommandIOData& /* data */) const {
  return false;
}

MasterAddPluginCommand::MasterAddPluginCommand(unsigned int send, unsigned int location_index, AudioPluginNode * plugin_node) :
  mSend(send), mLocationIndex(location_index), mPlugin(plugin_node)
{
}

MasterAddPluginCommand::~MasterAddPluginCommand() {
}

void MasterAddPluginCommand::execute(const Transport& /*transport*/) {
  master()->add_send_plugin(mSend, mLocationIndex, mPlugin);
}

bool MasterAddPluginCommand::store(CommandIOData& /* data */) const {
  return false;
}

