#ifndef DATAJOCKEY_SCHEDULER_HPP
#define DATAJOCKEY_SCHEDULER_HPP

#include "command.hpp"
#include "transport.hpp"
#include <jackringbuffer.hpp>

namespace DataJockey {
	class Scheduler {
		private:
			JackCpp::RingBuffer<Command *> mCommandsIn;
			JackCpp::RingBuffer<Command *> mCommandsOut;
		public:
			Scheduler();
			//this executes a command now, scheduler now owns this command
			void execute(Command * cmd);
			//this schedules a command based on the main transport
			void schedule(const TimePoint &time, const Command &cmd);
			//this clears the schedule [as soon as possible]
			void clear();
			//this is called by the audio callback
			//it executes commands based on the schedule
			//XXX memory leak, we don't yet clear out mCommandsOut;
			void execute_schedule(const Transport& transport);
	};
}

#endif
