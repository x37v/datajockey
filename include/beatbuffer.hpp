#ifndef DATAJOCKEY_BEATBUFFER_HPP
#define DATAJOCKEY_BEATBUFFER_HPP

#include "timepoint.hpp"
#include <string>
#include <stdexcept>
#include <vector>

namespace DataJockey {
	//XXX assuming 4/4 for now
	class BeatBuffer {
		public:
			BeatBuffer(std::string dataLocation) throw(std::runtime_error);
			double get_time(const TimePoint position);
			TimePoint get_position(double seconds);
			TimePoint end();
		private:
			void load(std::string dataLocation) throw(std::runtime_error);
			unsigned int mStartBeat;
			std::vector<double> mBeatData;
	};
}

#endif
