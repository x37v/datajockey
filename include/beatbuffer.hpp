#ifndef DATAJOCKEY_BEATBUFFER_HPP
#define DATAJOCKEY_BEATBUFFER_HPP

#include "timepoint.hpp"
#include <string>
#include <stdexcept>
#include <vector>

namespace DataJockey {
   namespace Audio {
      //XXX assuming 4/4 for now
      class BeatBuffer {
         public:
            BeatBuffer(std::string dataLocation) throw(std::runtime_error);
            double time_at_position(const TimePoint& position) const;
            TimePoint position_at_time(double seconds) const;
            //this one lets us give a 'last position' helper so we don't have to
            //search the whole thing
            TimePoint position_at_time(double seconds, const TimePoint& lastPos) const;
            TimePoint end() const;
            unsigned int start_offset() const;
            //setters
            void start_offset(unsigned int b);
         private:
            void load(std::string dataLocation) throw(std::runtime_error);
            //the start beat lets us offset the data
            //indicating where bar 0, beat 0 should in the data
            unsigned int mStartBeat;
            std::vector<double> mBeatData;
      };
   }
}

#endif
