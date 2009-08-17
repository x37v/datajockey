#ifndef DATAJOCKEY_COMMAND_CPP
#define DATAJOCKEY_COMMAND_CPP

#include "timepoint.hpp"

namespace DataJockey {
	class Command {
		public:
			virtual ~Command();
			const TimePoint& time_executed();
			void time_executed(TimePoint const & t);
			virtual void execute() = 0;
			//this is executed back in the main thread
			//after the command has come back from the audio thread
			//by default it does nothing
			virtual void execute_done();
		private:
			TimePoint mTimeExecuted;
	};
}

#endif
