#include "transport.hpp"

using namespace DataJockey;

Transport::Transport(){
	mPosition.at_bar(0);
	mIncrement = 1;
	bpm(120.0);
}

void Transport::setup(unsigned int sampleRate){
	mSampleRate = sampleRate;
}

void Transport::tick(){
	double index = mPosition.pos_in_beat() + mIncrement;
	while(index >= 1.0){
		index -= 1.0;
		mPosition.advance_beat();
		mPosition.pos_in_beat(index);
	}
}

const TimePoint& Transport::position() const { return mPosition; }
double Transport::bpm() const { return mBPM; }

void Transport::position(const TimePoint &pos){
	mPosition = pos;
}

void Transport::bpm(double val){
	mBPM = val;
	mIncrement = mBPM / (60.0 * (double)mSampleRate);
}
