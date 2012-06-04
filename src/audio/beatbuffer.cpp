#include "beatbuffer.hpp"
#include <yaml-cpp/yaml.h>
#include <vector>
#include <algorithm>
#include <fstream>

using namespace dj::audio;

template<typename Type>
Type linear_interp(Type v0, Type v1, double dist){
   return v0 + (v1 - v0) * dist;
}

BeatBuffer::BeatBuffer() {
   mStartBeat = 0;
}

bool BeatBuffer::load(std::string file_location) {
   clear();
   try {
      std::ifstream fin(file_location.c_str());
      YAML::Parser parser(fin);
      YAML::Node doc;
      parser.GetNextDocument(doc);

      const YAML::Node& locs = doc["beat_locations"];
      if (locs.Type() == YAML::NodeType::Sequence && locs.size() == 0) {
         return false;
      } else {
         //XXX just using the last in the list
         const YAML::Node& beats = (locs.Type() == YAML::NodeType::Sequence) ? locs[locs.size() - 1]["time_points"] : locs["time_points"];
         for (unsigned int i = 0; i < beats.size(); i++) {
            double beat;
            beats[i] >> beat;
            insert_beat(beat);
         }
      }
   } catch(...) {
      return false;
   }
   return true;
}

//XXX assuming 4/4
double BeatBuffer::time_at_position(const TimePoint& position) const {
   if(TimePoint::SECONDS == position.type())
      return position.seconds();
   else {
      if(position.valid()){
         int beat = position.beat() + position.bar() * 4;
         unsigned int size = mBeatData.size();
         beat += mStartBeat;
         //make sure we're in range!
         if (size == 0)
            return 0.0;
         else if(beat < 0)
            return 0.0;
         else if((unsigned int)beat >= size)
            return mBeatData.back();
         else if (position.pos_in_beat() <= 0 || 
               (position.pos_in_beat() > 0 && (unsigned int)beat + 1 >= size))
            return mBeatData[beat];
         else
            return linear_interp(mBeatData[beat], 
                  mBeatData[beat + 1], position.pos_in_beat());
      } else
         return 0.0;
   }
}

TimePoint BeatBuffer::position_at_time(double seconds) const {
   TimePoint pos;
   pos.at_bar(0);
   return position_at_time(seconds, pos);
}

TimePoint BeatBuffer::position_at_time(double seconds, const TimePoint& lastPos) const {
   TimePoint pos;
   unsigned int size = mBeatData.size();
   unsigned int start = mStartBeat;

   if (start >= size)
      start = 0;

   if(size > 0 && seconds >= 0){
      //make sure we aren't trying to advance before the start
      if (seconds <= mBeatData[start]) {
         pos.at_bar(start / 4, start % 4);
         return pos;
      } else if (seconds >= mBeatData.back()) {
         pos.at_bar(size / 4, size % 4);
         return pos;
      }

      //advance to the last position if it is before the time we are looking for
      //XXX what about TimePoint::SECONDS types?
      if(lastPos.type() == TimePoint::BEAT_BAR) {
         int beat = lastPos.beats_per_bar() * lastPos.bar() + lastPos.beat() + start;
         if (beat < static_cast<int>(size) && mBeatData[beat] <= seconds)
            start = beat;
      }

      for(unsigned int i = start; i < size; i++){
         //XXX assuming 4/4
         if(mBeatData[i] == seconds)
            pos.at_bar((i - start) / 4, (i - start) % 4);
         else if(mBeatData[i] > seconds){
            if(i == start){
               pos.at_bar(0);
            } else {
               unsigned int beat = i - 1 - start;
               double diff = mBeatData[i - start] - mBeatData[beat];
               if (diff != 0)
                  diff = (seconds - mBeatData[beat]) / diff;
               pos.at_bar(beat / 4, beat % 4, diff);
            }
            return pos;
         }
      }
      //XXX wasn't found.. should return end or invalid?
      return end_position();
   }
   return pos;
}

TimePoint BeatBuffer::end_position() const {
   TimePoint pos;
   unsigned int beats;
   if(mBeatData.size() == 0)
      return pos;
   //XXX assuming 4/4
   beats = mBeatData.size() - mStartBeat;
   pos.type(TimePoint::BEAT_BAR);
   pos.at_bar(beats / 4, beats % 4);
   return pos;
}

unsigned int BeatBuffer::start_offset() const {
   return mStartBeat;
}

void BeatBuffer::start_offset(unsigned int val){
   if(val < mBeatData.size())
      mStartBeat = val;
}

BeatBuffer::const_iterator BeatBuffer::begin() const { return mBeatData.begin(); }
BeatBuffer::iterator BeatBuffer::begin() { return mBeatData.begin(); }
BeatBuffer::const_iterator BeatBuffer::end() const { return mBeatData.end(); }
BeatBuffer::iterator BeatBuffer::end() { return mBeatData.end(); }

double BeatBuffer::operator[](unsigned int i) const { return mBeatData[i]; }

unsigned int BeatBuffer::length() const { return mBeatData.size(); }

void BeatBuffer::update_value(unsigned int index, double value) {
   if(index < mBeatData.size()) {
      mBeatData[index] = value;
   }
}

void BeatBuffer::update_value(iterator pos, double value) {
   if (pos != end())
      *pos = value;
}

void BeatBuffer::clear() {
   mBeatData.clear();
}

void BeatBuffer::insert_beat(double seconds) {
   //if it is empty or our new time point is greater than the last time point
   if (mBeatData.empty() || mBeatData.back() < seconds)
      mBeatData.push_back(seconds);
   else if(mBeatData.front() > seconds){
      mBeatData.push_front(seconds);
   } else {
      beat_list_t::iterator it = mBeatData.begin();
      //search for the position where the value is not less than our value to insert
      while(*it < seconds)
         it++;
      //insert inserts before the iterator
      mBeatData.insert(it, seconds);
   }
}

double BeatBuffer::median_difference() {
   if (mBeatData.size() < 2)
      return 0.1; //XXX is there a better value?

   std::vector<double> distances;
   for (unsigned int i = 0; i < mBeatData.size() - 1; i++)
      distances.push_back(mBeatData[i + 1] - mBeatData[i]);

   sort(distances.begin(), distances.end());

   if (distances.size() % 2 == 1)
      return distances[distances.size() / 2];
   else
      return (distances[distances.size() / 2] + distances[distances.size() / 2 + 1]) / 2.0;
}

#if 0

#include <iostream>
using std::cout;
using std::endl;

void printme(const BeatBuffer& buff) {
   for(BeatBuffer::const_iterator it = buff.begin(); it != buff.end(); it++) {
      cout << *it << endl;
   }
}

int main(int argc, char * argv[]){

   BeatBuffer b;
   TimePoint t(1,0,0);

   b.insert_beat(2.0);
   printme(b);

   b.insert_beat(4.0);
   b.insert_beat(5.0);
   b.insert_beat(6.0);
   b.insert_beat(7.0);

   cout << "printing" << endl;
   printme(b);

   BeatBuffer::iterator it = b.begin();
   *it = 0.1;
   cout << *it << endl;

   b.insert_beat(2.0);
   b.insert_beat(1.0);
   cout << "printing 2" << endl;
   printme(b);
   cout << "asdf" << endl;

   cout << b.time_at_position(t) << endl;
   cout << b.end_position().bar() << " : " << b.end_position().beat() << endl;

   t = b.position_at_time(6.75490263682522 );
   cout << "position : " << endl;
   cout << t.bar() << " : " << t.beat() << " : " << t.pos_in_beat() << endl;

   return 0;
}

#endif

