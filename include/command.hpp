#ifndef DATAJOCKEY_COMMAND_CPP
#define DATAJOCKEY_COMMAND_CPP

#include "timepoint.hpp"

namespace DataJockey {
	class Command {
		public:
			virtual ~Command();
			TimePoint time_executed();
			void time_executed(TimePoint const & t);
			virtual void execute() = 0;
		private:
			TimePoint mTimeExecuted;
	};
}

#endif
