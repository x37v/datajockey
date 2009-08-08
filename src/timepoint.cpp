#include "timepoint.hpp"

using namespace DataJockey;

TimePoint::TimePoint(){
	mType = BEAT_BAR;
	invalidate();
}

//getters
TimePoint::time_type TimePoint::type() const { return mType; }
int TimePoint::bar() const { return mBar; }
int TimePoint::beat() const { return mBeat; }
double TimePoint::pos_in_beat() const { return mPosInBeat; }
float TimePoint::beats_per_bar() const { return mBeatsPerBar; }
float TimePoint::beat_type() const { return mBeatType; }
double TimePoint::seconds() const { return mSeconds; }
bool TimePoint::valid() const {
	if(mType == SECONDS)
		return true;
	else {
		if (mBar < 0 || mBeat < 0)
			return false;
		else
			return true;
	}
}

//setters
void TimePoint::type(TimePoint::time_type t){ mType = t; }
void TimePoint::bar(int val){ mBar = val; }
void TimePoint::beat(int val){ mBeat = val; }
void TimePoint::pos_in_beat(double val){ mPosInBeat = val; }
void TimePoint::beats_per_bar(float val){ mBeatsPerBar = val; }
void TimePoint::beat_type(float val){ mBeatType = val; }
void TimePoint::seconds(double val){ mSeconds = val; }
void TimePoint::at_bar(int newBar, int newBeat, double newPos){
	bar(newBar);
	beat(newBeat);
	pos_in_beat(newPos);
}

//XXX assuming 4/4
void TimePoint::at_beat(int newBeat, double newPos){
	bar(newBeat / 4);
	beat(newBeat % 4);
	pos_in_beat(newPos);
}

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
