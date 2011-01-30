#include "timepoint.hpp"
#include <stdlib.h>

using namespace DataJockey::Audio;

TimePoint::TimePoint(time_type t){
   mType = t;
   //default type is 4/4
   mBeatsPerBar = mBeatType = 4;
   invalidate();
}

TimePoint::TimePoint(double sec) {
   mType = SECONDS;
   seconds(sec);
}

TimePoint::TimePoint(int bar, unsigned int beat, double pos_in_beat) {
   //default type is 4/4
   mBeatsPerBar = mBeatType = 4;
   mType = BEAT_BAR;
   at_bar(bar, beat, pos_in_beat);
}

TimePoint::TimePoint(const TimePoint& source) {
   mType = source.mType;
   mBar = source.mBar;
   mBeat = source.mBeat;
   mPosInBeat = source.mPosInBeat;
   mBeatsPerBar = source.mBeatsPerBar;
   mBeatType = source.mBeatType;
   mSeconds = source.mSeconds;
}

TimePoint& TimePoint::operator=(const TimePoint& source) {
   mType = source.mType;
   mBar = source.mBar;
   mBeat = source.mBeat;
   mPosInBeat = source.mPosInBeat;
   mBeatsPerBar = source.mBeatsPerBar;
   mBeatType = source.mBeatType;
   mSeconds = source.mSeconds;
   return *this;
}

//getters
TimePoint::time_type TimePoint::type() const { return mType; }
int TimePoint::bar() const { return mBar; }
unsigned int TimePoint::beat() const { return mBeat; }
double TimePoint::pos_in_beat() const { return mPosInBeat; }
unsigned int TimePoint::beats_per_bar() const { return mBeatsPerBar; }
unsigned int TimePoint::beat_type() const { return mBeatType; }
double TimePoint::seconds() const { return mSeconds; }

bool TimePoint::valid() const {
   if(mType == SECONDS)
      return true;
   else {
      if (mPosInBeat < 0)
         return false;
      else
         return true;
   }
}

//setters
void TimePoint::type(TimePoint::time_type t){ mType = t; }
void TimePoint::bar(int val){ mBar = val; }
void TimePoint::beat(unsigned int val){ mBeat = val; }
void TimePoint::pos_in_beat(double val){ mPosInBeat = val; }

void TimePoint::beats_per_bar(unsigned int val){ 
   if(mBeatsPerBar > 0)
      mBeatsPerBar = val; 
}

void TimePoint::beat_type(unsigned int val){ 
   if(mBeatType > 0)
      mBeatType = val; 
}

void TimePoint::seconds(double val){ mSeconds = val; }

void TimePoint::at_bar(int newBar, unsigned int newBeat, double newPos){
   //fix up range!
   while(newPos >= 1.0){
      newBeat += 1;
      newPos -= 1.0;
   }
   if(newPos < 0)
      newPos = 0;
   //TODO does this still work with negative bar?
   while(newBeat >= mBeatsPerBar){
      newBeat -= mBeatsPerBar;
      newBar += 1;
   }
   bar(newBar);
   beat(newBeat);
   pos_in_beat(newPos);
}

void TimePoint::at_beat(int newBeat, double newPos){
   //fix up range!
   while(newPos >= 1.0){
      newBeat += 1;
      newPos -= 1.0;
   }
   if(newPos < 0)
      newPos = 0;
   bar(newBeat / mBeatsPerBar);
   beat(abs(newBeat) % mBeatsPerBar);
   pos_in_beat(newPos);
}

void TimePoint::advance_beat(){
   mBeat += 1;
   while(mBeat >= mBeatsPerBar){
      mBar += 1;
      mBeat -= mBeatsPerBar;
   }
}

void TimePoint::invalidate(){
   mSeconds = -1;
   mBar = mBeat = 0;
   mPosInBeat = -1; 
}

bool TimePoint::operator==(const TimePoint &other) const {
   if(other.valid() && valid()){
      if(other.type() == SECONDS){ 
         if(this->type() != SECONDS)
            return false;
         else if (this->seconds() != other.seconds())
            return false;
         else
            return true;
      } else {
         //XXX we should do transformations between time
         //signatures here..
         if(this->type() != BEAT_BAR)
            return false;
         else if(this->bar() != other.bar() ||
               this->beat() != other.beat() ||
               this->pos_in_beat() != other.pos_in_beat() ||
               this->beat_type() != other.beat_type() ||
               this->beats_per_bar() != other.beats_per_bar())
            return false;
         else
            return true;
      }
   } else
      return false;
}

bool TimePoint::operator!=(const TimePoint &other) const {
   return !(*this == other);
}

bool TimePoint::operator<(const TimePoint &other) const {
   if(other.valid() && valid()){
      if(other.type() == SECONDS){ 
         if(this->type() != SECONDS)
            return false;
         else if (this->seconds() < other.seconds())
            return true;
         else
            return false;
      } else {
         if(this->type() != BEAT_BAR)
            return false;
         else {
            //XXX ignoring beat_type!  Assuming x/4
            unsigned int this_beats = this->beat() + this->bar() * this->beats_per_bar();
            unsigned int other_beats = other.beat() + other.bar() * other.beats_per_bar();
            if(this_beats < other_beats || 
                  (this_beats == other_beats && this->pos_in_beat() < other.pos_in_beat()))
               return true;
            else 
               return false;
         }
      }
   } else
      return false;
}

bool TimePoint::operator>(const TimePoint &other) const {
   if(other.valid() && valid()){
      if(other.type() == SECONDS){ 
         if(this->type() != SECONDS)
            return false;
         else if (this->seconds() > other.seconds())
            return true;
         else
            return false;
      } else {
         if(this->type() != BEAT_BAR)
            return false;
         else {
            //XXX ignoring beat_type!  Assuming x/4
            unsigned int this_beats = this->beat() + this->bar() * this->beats_per_bar();
            unsigned int other_beats = other.beat() + other.bar() * other.beats_per_bar();
            if(this_beats > other_beats || 
                  (this_beats == other_beats && this->pos_in_beat() > other.pos_in_beat()))
               return true;
            else 
               return false;
         }
      }
   } else
      return false;
}

bool TimePoint::operator<=(const TimePoint &other) const {
   return ((*this < other) || (*this == other));
}

bool TimePoint::operator>=(const TimePoint &other) const {
   return ((*this > other) || (*this == other));
}

const TimePoint TimePoint::operator+(const TimePoint &other) const {
   TimePoint ret(*this);
   if (other.type() == type()) {
      if (type() == SECONDS) {
         double sec = seconds() + other.seconds(); 
         ret.seconds(sec);
      } else {
         //TODO
      }
   } else {
      //TODO
   }
   return ret;
}

const TimePoint TimePoint::operator-(const TimePoint &other) const {
   TimePoint ret(*this);
   if (other.type() == type()) {
      if (type() == SECONDS) {
         //TODO should we not allow negative?
         ret.seconds(seconds() - other.seconds()); 
      } else {
         //XXX ignoring beat_type!  Assuming x/4
         int this_beats = this->beat() + this->bar() * this->beats_per_bar();
         int other_beats = other.beat() + other.bar() * other.beats_per_bar();
         int diff  = this_beats - other_beats;
         double pos_diff = this->pos_in_beat() - other.pos_in_beat();
         while (pos_diff < 0) {
            diff -= 1;
            pos_diff += 1.0;
         }
         if (diff < 0)
            diff = 0;
         ret.at_beat(diff, pos_diff);
      }
   } else {
      //TODO
   }
   return ret;
}

