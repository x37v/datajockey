#ifndef DATAJOCKEY_BEATBUFFER_HPP
#define DATAJOCKEY_BEATBUFFER_HPP

#include "timepoint.hpp"
#include <string>
#include <stdexcept>
#include <deque>

namespace DataJockey {
   namespace Audio {
      //XXX assuming 4/4 for now
      class BeatBuffer {
         public:
            typedef std::deque<double> beat_list_t;
            typedef beat_list_t::const_iterator const_iterator;
            typedef beat_list_t::iterator iterator;

            BeatBuffer();
            double time_at_position(const TimePoint& position) const;
            TimePoint position_at_time(double seconds) const;
            //this one lets us give a 'last position' helper so we don't have to
            //search the whole thing
            TimePoint position_at_time(double seconds, const TimePoint& lastPos) const;
            TimePoint end_position() const;
            unsigned int start_offset() const;

            BeatBuffer::const_iterator begin() const;
            BeatBuffer::iterator begin();
            BeatBuffer::const_iterator end() const;
            BeatBuffer::iterator end();

            unsigned int length() const;

            //setters realtime safe
            void start_offset(unsigned int b);

            //update the value of the beat at the index given
            void update_value(unsigned int index, double value);
            void update_value(iterator pos, double value);

            //setters not realtime safe
            void clear();
            void insert_beat(double seconds);

         private:
            //the start beat lets us offset the data
            //indicating where bar 0, beat 0 should in the data
            unsigned int mStartBeat;
            beat_list_t mBeatData;
      };
   }
}

#endif
