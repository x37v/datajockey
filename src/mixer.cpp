#include "mixer.hpp"

using namespace DataJockey;

Mixer::Mixer(unsigned int numPlayers){
	mCueBuffer = NULL;
	for(unsigned int i = 0; i < numPlayers; i++)
		add_player(new Player);
}

Mixer::~Mixer(){
	//clean up!
	if(mCueBuffer != NULL){
		delete [] mCueBuffer[0];
		delete [] mCueBuffer[1];
		delete [] mCueBuffer;
	}
	for(unsigned int i = 0; i < mPlayerBuffers.size(); i++){
		delete [] mPlayerBuffers[i];
	}
	
}

void Mixer::setup_audio(
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

	//set up the players and their buffers
	for(unsigned int i = 0; i < mPlayers.size(); i++){
		float ** sampleBuffer = new float*[2];
		sampleBuffer[0] = new float[maxBufferLen];
		sampleBuffer[1] = new float[maxBufferLen];
		mPlayers[i]->setup_audio(sampleRate, maxBufferLen);
		mPlayerBuffers.push_back(sampleBuffer);
	}
}

void Mixer::add_player(Player * p){
	mPlayers.push_back(p);
}

void Mixer::audio_compute_and_fill(
		JackCpp::AudioIO::audioBufVector outBufferVector,
		unsigned int numFrames){
	//set up players
	for(unsigned int p = 0; p < mPlayers.size(); p++)
		mPlayers[p]->audio_pre_compute(numFrames, mPlayerBuffers[p]);

	//compute their samples [and do other stuff]
	for(unsigned int frame = 0; frame < numFrames; frame++){
		//zero out the cue buffer
		for(unsigned int chan = 0; chan < 2; chan++)
			mCueBuffer[chan][frame] = 0.0;
		//zero out the output buffers
		for(unsigned int chan = 0; chan < 4; chan++)
			outBufferVector[chan][frame] = 0.0;
		for(unsigned int j = 0; j < mPlayers.size(); j++){
			mPlayers[j]->audio_compute_frame(frame, mPlayerBuffers[j], NULL);
		}
	}

	//finalize each player, and copy its data out
	for(unsigned int p = 0; p < mPlayers.size(); p++){
		mPlayers[p]->audio_post_compute(numFrames, mPlayerBuffers[p]);
		mPlayers[p]->audio_fill_output_buffers(numFrames, mPlayerBuffers[p], mCueBuffer);
		//actually copy the data to the output
		for(unsigned int j = 0; j < numFrames; j++){
			for(unsigned int chan = 0; chan < 2; chan++){
				outBufferVector[chan][j] += mPlayerBuffers[p][chan][j];
				outBufferVector[chan + 2][j] += mCueBuffer[chan][j];
			}
		}
	}
}
