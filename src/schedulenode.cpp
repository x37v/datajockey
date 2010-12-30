#include "schedulenode.hpp"

using namespace DataJockey::Audio;

ScheduleNode::ScheduleNode(Command * c, const TimePoint& t){
   next = prev = NULL;
   command = c;
   time = t;
}

ScheduleNode::~ScheduleNode(){
   delete command;
}

