#include "player.hpp"
#include <math.h>
#define RUBBERBAND_WINDOW_SIZE 64
#define MIN(x,y) ((x) < (y) ? (x) : (y))
using namespace DataJockey;

Player::Player(){
	//states
	mPlayState = PAUSE;
	mOutState = CUE;
	mStretchMethod = RUBBER_BAND;
	mMute = false;
	mSync = false;
	mLoop = false;

	//continuous
	mVolume = 1.0;
	mPlaySpeed = 1.0;

	mVolumeBuffer = NULL;
	mAudioBuffer = NULL;
	mSampleIndex = 0;
	mSampleIndexResidual = 0.0;
	mRubberBandStretcher = NULL;

	//by default we start at the beginning of the audio
	mStartPosition.at_bar(0);
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
}


//the audio computation methods
//setup for audio computation
void Player::audio_pre_compute(unsigned int numFrames, float ** mixBuffer){
	if(mSampleIndex + mSampleIndexResidual >= mAudioBuffer->length())
		mPlayState = PAUSE;

	if(mStretchMethod == RUBBER_BAND){
		mRubberBandStretcher->setTimeRatio(1.0 / mPlaySpeed);
		if(mPlayState == PLAY){
			while(mRubberBandStretcher->available() < numFrames){
				unsigned int winSize = MIN(RUBBERBAND_WINDOW_SIZE, numFrames);
				for(unsigned int i = 0; i < 2; i++){
					for(unsigned int j = 0; j < winSize; j++){
						mixBuffer[i][j] = mAudioBuffer->sample(i, mSampleIndex + j);
					}
				}
				mSampleIndex += winSize;
				mRubberBandStretcher->process(mixBuffer, winSize, false);
			}
		}
		mSampleIndexResidual = 0;
	}
}

//actually compute one frame, filling an internal buffer
//syncing to the transport if mSync == true
void Player::audio_compute_frame(unsigned int frame, float ** mixBuffer, 
		const Transport * transport){
	//zero out the frame;
	mixBuffer[0][frame] = mixBuffer[1][frame] = 0.0;
	//compute the volume
	mVolumeBuffer[frame] = mMute ? 0.0 : mVolume;

	//compute the actual frame
	if(mPlayState == PLAY){
		switch(mStretchMethod){
			case PLAY_RATE:
				for(unsigned int i = 0; i < 2; i++){
					mixBuffer[i][frame] = 
						mAudioBuffer->sample(i, mSampleIndex, mSampleIndexResidual);
				}
				mSampleIndexResidual += mPlaySpeed;
				mSampleIndex += floor(mSampleIndexResidual);
				mSampleIndexResidual -= floor(mSampleIndexResidual);
				break;
			case RUBBER_BAND:
				//XXX can't this be simplified?
				float *tmp[2];
				float right;
				float left;
				tmp[0] = &right;
				tmp[1] = &left;

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
void Player::audio_post_compute(unsigned int numFrames, float ** mixBuffer){
}

//actually fill the output vectors
void Player::audio_fill_output_buffers(unsigned int numFrames,
		float ** mixBuffer, float ** cueBuffer){

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
Player::play_state_t Player::play_state(){ return mPlayState; }
Player::out_state_t Player::out_state(){ return mOutState; }
Player::stretch_method_t Player::stretch_method(){ return mStretchMethod; }
bool Player::muted(){ return mMute; }
bool Player::syncing(){ return mSync; }
bool Player::looping(){ return mLoop; }
double Player::volume(){ return mVolume; }
double Player::play_speed(){ return mPlaySpeed; }
TimePoint Player::position(){ return mPosition; }
TimePoint Player::start_position(){ return mStartPosition; }
TimePoint Player::end_position(){ return mEndPosition; }
TimePoint Player::loop_start_position(){ return mLoopStartPosition; }
TimePoint Player::loop_end_position(){ return mLoopEndPosition; }

//setters
void Player::play_state(play_state_t val){
	mPlayState = val;
}

void Player::out_state(out_state_t val){
	mOutState = val;
}

void Player::stretch_method(stretch_method_t val){
}

void Player::mute(bool val){
	mMute = val;
}

void Player::sync(bool val){
	mSync = val;
}

void Player::loop(bool val){
}

void Player::volume(double val){
	mVolume = val;
}

void Player::play_speed(double val){
	mPlaySpeed = val;
}

void Player::position(const TimePoint &val){
	mPosition = val;
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
	mPosition = mStartPosition;
}


//misc
void Player::position_relative(TimePoint amt){
}

void Player::Player::play_speed_relative(double amt){
	mPlaySpeed += amt;
}

void Player::Player::Player::volume_relative(double amt){
	mVolume += amt;
}

//command stuff
//
PlayerCommand::PlayerCommand(Player * player){
	mPlayer = player;
}

Player * PlayerCommand::player(){ return mPlayer; }
TimePoint PlayerCommand::position_executed(){ return mPositionExecuted; }
void PlayerCommand::position_executed(TimePoint const & t){
	mPositionExecuted = t;
}

PlayerStateCommand::PlayerStateCommand(Player * player, action_t action) :
	PlayerCommand(player)
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

PlayerDoubleCommand::PlayerDoubleCommand(Player * player, 
		action_t action, double value) :
	PlayerCommand(player)
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

PlayerLoadCommand::PlayerLoadCommand(Player * player, AudioBuffer * buffer) : 
	PlayerCommand(player)
{
	mAudioBuffer = buffer;
}

void PlayerLoadCommand::execute(){
	Player * p = player(); 
	if(p != NULL){
		//store the time executed
		position_executed(p->position());
		//execute the action
		p->audio_buffer(mAudioBuffer);
	}
}


PlayerPositionCommand::PlayerPositionCommand(Player * player, 
		position_t target, TimePoint & timepoint) : 
	PlayerCommand(player)
{
	mTimePoint = timepoint;
	mTarget = target;
}

void PlayerPositionCommand::execute(){
	Player * p = player(); 
	if(p != NULL){
		//store the time executed
		position_executed(p->position());
		//execute the action
		switch(mTarget){
			case PLAY:
				p->position(mTimePoint);
				break;
			case PLAY_RELATIVE:
				p->position_relative(mTimePoint);
				break;
			case START:
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
	}
}
