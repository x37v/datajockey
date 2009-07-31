#ifndef DATAJOCKEY_TRANSPORT_HPP
#define DATAJOCKEY_TRANSPORT_HPP

#include "timepoint.hpp"

namespace DataJockey {
	class Transport {
		public:
			Transport();
			//const TimePoint * time_point();
			virtual void start() = 0;
			virtual void stop() = 0;
		private:
			//the current time point of this transport
			TimePoint mTimePoint;
	};
}

#endif
