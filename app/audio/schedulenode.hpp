#ifndef DATAJOCKEY_SCHEDULE_NODE_HPP
#define DATAJOCKEY_SCHEDULE_NODE_HPP

#include "command.hpp"
#include "timepoint.hpp"

namespace djaudio {
  class ScheduleNode {
    public:
      ScheduleNode(Command * c, const TimePoint& t);
      ~ScheduleNode();
      ScheduleNode * next;
      ScheduleNode * prev;
      Command * command;
      TimePoint time;
  };
}

#endif
