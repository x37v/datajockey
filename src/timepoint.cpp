#include "timepoint.hpp"

using namespace DataJockey;

TimePoint::TimePoint(){
	mType = BEAT_BAR;
	invalidate();
}

//getters
TimePoint::time_type TimePoint::type(){ return mType; }
int TimePoint::bar(){ return mBar; }
int TimePoint::beat(){ return mBeat; }
double TimePoint::pos_in_beat(){ return mPosInBeat; }
float TimePoint::beats_per_bar(){ return mBeatsPerBar; }
float TimePoint::beat_type(){ return mBeatType; }
double TimePoint::seconds(){ return mSeconds; }

//setters
void TimePoint::type(TimePoint::time_type t){ mType = t; }
void TimePoint::bar(int val){ mBar = val; }
void TimePoint::beat(int val){ mBeat = val; }
void TimePoint::pos_in_beat(double val){ mPosInBeat = val; }
void TimePoint::beats_per_bar(float val){ mBeatsPerBar = val; }
void TimePoint::beat_type(float val){ mBeatType = val; }
void TimePoint::seconds(double val){ mSeconds = val; }

void TimePoint::invalidate(){
	mSeconds = -1;
	mBar = mBeat = mPosInBeat = -1; 
	//default type is 4/4
	mBeatsPerBar = mBeatType = 4;
}

/*

#include <iostream>
using std::cout;
using std::endl;

int main(){
	TimePoint t;
	t.type(TimePoint::SECONDS);
	cout << t.type() << endl;
	return 0;
}

*/
