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
			class ScheduleNode;
			class AddCommand;
			//this is the main schedule, relative to the transport
			ScheduleNode * mSchedule;
			//a pointer to the [guessed] current location in the schedule
			ScheduleNode * mScheduleCur;
			//a pointer to the last executed location in the schedule
			//we need this because we might not execute the schedule at every sample
			//so we might have to execute scheduled items within a range of time
			ScheduleNode * mScheduleLast;
		public:
			Scheduler();
			//this executes a command now, scheduler now owns this command
			void execute(Command * cmd);
			//this schedules a command based on the main transport
			//returns an ID for this node
			unsigned int schedule(const TimePoint &time, Command * cmd);
			//removes a scheduled node
			void remove(unsigned int id);
			//this clears the schedule [as soon as possible]
			void clear();
			//this is called by the audio callback
			//it executes commands based on the schedule
			//XXX memory leak, we don't yet clear out mCommandsOut;
			void execute_schedule(const Transport& transport);
			//this is called back in the main thread, 
			//clearing the command out buffer and executing their done actions
			void execute_done_actions();
			//this is called to invalid the mScheduleLast and Cur pointer
			//incase we've jumped around in the transport
			void invalidate_schedule_pointers();

			friend class AddCommand;
		private:
			//adds a node to the schedule
			void add(ScheduleNode * node);
			//removes a node from the schedule
			void remove(ScheduleNode * node);

			class ScheduleNode {
				public:
					ScheduleNode(Command * c, const TimePoint& t);
					ScheduleNode * next;
					ScheduleNode * prev;
					Command * command;
					TimePoint time;
			};

			class AddCommand : public Command {
				public:
					AddCommand(Scheduler * scheduler, ScheduleNode * node);
					virtual void execute();
				private:
					Scheduler * mScheduler;
					ScheduleNode * mNode;
			};
	};
}

#endif
