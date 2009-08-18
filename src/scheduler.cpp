#include "scheduler.hpp"
#include "timepoint.hpp"
#include <iostream>

using std::cerr;
using std::endl;

using namespace DataJockey;

Scheduler::Scheduler() : 
	mCommandsIn(1024), mCommandsOut(1024)
{
	mSchedule = NULL; 
	invalidate_schedule_pointers();
}

void Scheduler::execute(Command * cmd){
	if(mCommandsIn.getWriteSpace())
		mCommandsIn.write(cmd);
	else {
		cerr << "no space in command queue, deleting command" << endl;
		delete cmd;
	}
}

unsigned int Scheduler::schedule(const TimePoint &time, Command * cmd){
	ScheduleNode * node = new ScheduleNode(cmd, time);;
	AddCommand * addCmd = new AddCommand(this, node);
	execute(addCmd);
	//XXX not implemented
	return 0;
}

void remove(unsigned int id){
	//XXX not implemented
}

void Scheduler::clear(){
	//XXX not implemented
}

void Scheduler::execute_schedule(const Transport& transport){
	//while there are commands to be executed
	//execute them, noteing the time, 
	//and write them to the out buffer
	while(mCommandsIn.getReadSpace()){
		Command * cmd;
		TimePoint pos = transport.position();
		mCommandsIn.read(cmd);
		cmd->execute();
		cmd->time_executed(pos);
		if(mCommandsOut.getWriteSpace())
			mCommandsOut.write(cmd);
		//XXX what if there is no write space?
	}
	//actually eval the scheuldule
	if(mSchedule == NULL)
		return;
	if(mScheduleCur == NULL)
		mScheduleCur = mSchedule;

	//backtrack, if we have a valid last time, we go to the
	//node just past the last time
	//otherwise we go to the node closest to the front that
	//has time >= transport.position()
	if(mLastScheduledTime.valid()){
		//here we are concerned with anything that has a timepoint
		//larger than the last time and less than or equal to the 
		//transport time
		while(mScheduleCur->prev != NULL && 
				mScheduleCur->prev->time > mLastScheduledTime){
			mScheduleCur = mScheduleCur->prev;
		}
		if(mScheduleCur->time > mLastScheduledTime &&
				mScheduleCur->time <= transport.position()){
			//iterate through the schedule, executing as we go
			//until our next node is either null, or has a time point
			//greater than the transport position
			while(true){
				//execute
				mScheduleCur->command->execute();
				//advance or break
				if(mScheduleCur->next == NULL ||
						mScheduleCur->next->time > transport.position()){
					break;
				} else
					mScheduleCur = mScheduleCur->next;
			}
		}
	} else {
		//here we only care about stuff that is scheduled exactly at the
		//transport time
		while(mScheduleCur->prev != NULL && 
				mScheduleCur->prev->time >= transport.position()){
			mScheduleCur = mScheduleCur->prev;
		}
		//execute everything at the time of the transport
		while(mScheduleCur->time == transport.position()){
			mScheduleCur->command->execute();
			if(mScheduleCur->next == NULL)
				break;
			else
				mScheduleCur = mScheduleCur->next;
		}
	}
	//store the current time as the 'last time' for next time we execute_schedule
	mLastScheduledTime = transport.position();
}

void Scheduler::execute_done_actions(){
	//while there are commands to be executed
	//execute them, noteing the time, 
	//and write them to the out buffer
	while(mCommandsOut.getReadSpace()){
		Command * cmd;
		mCommandsOut.read(cmd);
		cmd->execute_done();
	}
}

void Scheduler::invalidate_schedule_pointers(){
	mScheduleCur = NULL;
	mLastScheduledTime.invalidate();
}

void Scheduler::add(ScheduleNode * node){
	//if this is the first we set the schedule to this node
	if(mSchedule == NULL)
		mSchedule = node;
	else if (mSchedule->time > node->time){
		//if the first node has a later time than this new node
		//place this new node at the front
		node->next = mSchedule;
		mSchedule->prev = node;
		mSchedule = node;
	} else {
		ScheduleNode * cur = mSchedule;
		//find our place in the schedule
		while(cur->next != NULL && cur->next->time < node->time)
			cur = cur->next;
		node->prev = cur;
		node->next = cur->next;
		cur->next = node;
		//if there is a next node set its prev to us
		if(node->next)
			node->next->prev = node;
	}
}

void Scheduler::remove(ScheduleNode * node){
	if(node->next)
		node->next->prev = node->prev;
	if(node->prev)
		node->prev->next = node->next;
}

Scheduler::ScheduleNode::ScheduleNode(Command * c, const TimePoint& t){
	next = prev = NULL;
	command = c;
	time = t;
}

Scheduler::AddCommand::AddCommand(Scheduler * scheduler, ScheduleNode * node){
	mScheduler = scheduler;
	mNode = node;
}

void Scheduler::AddCommand::execute(){
	mScheduler->add(mNode);
}
