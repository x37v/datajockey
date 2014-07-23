#ifndef DATAJOCKEY_SCHEDULER_HPP
#define DATAJOCKEY_SCHEDULER_HPP

#include "command.hpp"
#include "transport.hpp"
#include "schedulenode.hpp"
#include <jackringbuffer.hpp>
#include <map>
#include <QMutex>
#include <QMutexLocker>
#include <QList>

namespace djaudio {
  class Scheduler {
    public:
      typedef unsigned long node_id_t;
    private:
      JackCpp::RingBuffer<Command *> mCommandsIn;
      JackCpp::RingBuffer<Command *> mCommandsOut;
      QList<Command *> mCommandsComplete;
      class AddCommand;
      //this is the main schedule, relative to the transport
      ScheduleNode * mSchedule;
      //a pointer to the [guessed] current location in the schedule
      ScheduleNode * mScheduleCur;
      //this is is the last spot that the schedule was run
      //it can be invalidated if we jump around in the transport
      TimePoint mLastScheduledTime;
      node_id_t mNodeIndex;
      //this maps the index given by the schedule for a node to a pointer
      //to the node itself
      std::map<node_id_t, ScheduleNode *> mNodeMap;
      //make sure that don't try to write to the command queue from more than one thread once
      QMutex mExecuteMutex;
    public:
      Scheduler();
      //this executes a command now, scheduler now owns this command
      void execute(Command * cmd);
      //this schedules a command based on the main transport
      //returns an ID for this node
      node_id_t schedule(const TimePoint &time, Command * cmd);
      //removes a scheduled node
      void remove(node_id_t id);
      //this clears the schedule [as soon as possible]
      void clear();
      //this is called by the audio callback
      //it executes commands based on the schedule
      void execute_schedule(const Transport& transport);
      //this is called back in the main thread, 
      //clearing the command out buffer and executing their done actions
      void execute_done_actions();
      //this is called to invalid the mScheduleLast and Cur pointer
      //incase we've jumped around in the transport
      void invalidate_schedule_pointers();

      //this is only called from the audio thread, takes ownership of the command
      void execute_immediately(Command * cmd, const Transport& transport);

      //this must be called in the same thread as execute_done_actions
      //get a command off the complete command list
      Command * pop_complete_command();

      friend class AddCommand;
    private:
      //adds a node to the schedule
      void add(ScheduleNode * node);
      //removes a node from the schedule
      void remove(ScheduleNode * node);

      class SchedulerCommand : public Command {
        public:
          SchedulerCommand(Scheduler * scheduler);
        protected:
          Scheduler * scheduler() const;
        private:
          Scheduler * mScheduler;
      };

      class AddCommand : public SchedulerCommand {
        public:
          AddCommand(Scheduler * scheduler, ScheduleNode * node);
          virtual void execute(const Transport& transport);
          virtual bool store(CommandIOData& data) const;
        private:
          ScheduleNode * mNode;
      };

      class RemoveCommand : public SchedulerCommand {
        public:
          RemoveCommand(Scheduler * scheduler, ScheduleNode * node);
          virtual void execute(const Transport& transport);
          virtual bool store(CommandIOData& data) const;
        private:
          ScheduleNode * mNode;
      };

  };
}

#endif
