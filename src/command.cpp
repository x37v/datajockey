#include "command.hpp"
using namespace DataJockey;

Command::~Command(){
}

const TimePoint& Command::time_executed(){ return mTimeExecuted; }
void Command::time_executed(TimePoint const & t){
	mTimeExecuted = t;
}
