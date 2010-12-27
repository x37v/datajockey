#include "schedulenode.hpp"

using namespace DataJockey::Internal;

ScheduleNode::ScheduleNode(Command * c, const TimePoint& t){
	next = prev = NULL;
	command = c;
	time = t;
}

ScheduleNode::~ScheduleNode(){
	delete command;
}

