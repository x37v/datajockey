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

namespace {
   //find the mid point between the previous and next values
   //find the difference between the data we have and that value, add 1/2 of that to the point
   double smoothed_point(double cur, double prev, double next) {
      double mid = (next - prev) / 2.0 + prev;
      return cur + (mid - cur) / 2.0;
   }
}

BeatBuffer::BeatBuffer() {
}

BeatBuffer::~BeatBuffer() {
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
   if(size > 0 && seconds >= 0){
      //make sure that our time is within our range
      if (seconds <= mBeatData[0]) {
         pos.at_bar(0);
         return pos;
      } else if (seconds >= mBeatData.back()) {
         pos.at_bar(size / 4, size % 4);
         return pos;
      }

      /*
       * temporarily ditch this
       * XXX
       *
      //advance to the last position if it is before the time we are looking for
      //XXX what about TimePoint::SECONDS types?
      if(lastPos.type() == TimePoint::BEAT_BAR) {
         int beat = lastPos.beats_per_bar() * lastPos.bar() + lastPos.beat() + start;
         if (beat < static_cast<int>(size) && mBeatData[beat] <= seconds)
            start = beat;
      }
      */

      for(unsigned int i = 1; i < size; i++){
         //XXX assuming 4/4
         if(mBeatData[i] == seconds) {
            pos.at_bar(i / 4, i % 4);
            return pos;
         } else if (mBeatData[i] > seconds){
            unsigned int beat = i - 1;
            double diff = mBeatData[i] - mBeatData[beat];
            if (diff != 0)
               diff = (seconds - mBeatData[beat]) / diff;
            pos.at_bar(beat / 4, beat % 4, diff);
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
   beats = mBeatData.size();
   pos.type(TimePoint::BEAT_BAR);
   pos.at_bar(beats / 4, beats % 4);
   return pos;
}

BeatBuffer::const_iterator BeatBuffer::begin() const { return mBeatData.begin(); }
BeatBuffer::iterator BeatBuffer::begin() { return mBeatData.begin(); }
BeatBuffer::const_iterator BeatBuffer::end() const { return mBeatData.end(); }
BeatBuffer::iterator BeatBuffer::end() { return mBeatData.end(); }

double BeatBuffer::at(unsigned int i) const { return mBeatData[i]; }
double BeatBuffer::operator[](unsigned int i) const { return at(i); }

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

void BeatBuffer::smooth(unsigned int iterations) {
   if (mBeatData.empty() || mBeatData.size() < 4)
      return;
   for (unsigned int i = 0; i < iterations; i++) {
      //go forward, the backwards
      if (i % 2 == 0) {
         for (unsigned int j = 1; j < mBeatData.size() - 1; j++)
            mBeatData[j] = smoothed_point(mBeatData[j], mBeatData[j - 1], mBeatData[j + 1]);
      } else {
         for (unsigned int j = mBeatData.size() - 2; j > 0; j--)
            mBeatData[j] = smoothed_point(mBeatData[j], mBeatData[j - 1], mBeatData[j + 1]);
      }
   }
}

double BeatBuffer::median_difference() {
   double median, mean;
   median_and_mean(median, mean);
   return median;
}

void BeatBuffer::median_and_mean(double& median, double& mean) {
   if (mBeatData.size() < 2) {
      //XXX is there a better value?
      median = mean = 0.1;
   }

   std::vector<double> distances;
   double sum = 0.0;
   for (unsigned int i = 0; i < mBeatData.size() - 1; i++) {
      double dist = mBeatData[i + 1] - mBeatData[i];
      distances.push_back(dist);;
      sum += dist;
   }

   mean = sum / distances.size();

   //median
   sort(distances.begin(), distances.end());
   if (distances.size() % 2 == 1)
      median = distances[distances.size() / 2];
   else
      median = (distances[distances.size() / 2] + distances[distances.size() / 2 + 1]) / 2.0;
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

