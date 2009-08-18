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
	//XXX not quite correct i don't think
	if(mSchedule != NULL){
		if(mScheduleCur == NULL)
			mScheduleCur = mSchedule;
		if(mScheduleCur->time > transport.position())
			return;
		//set our 'last' pointer if it isn't set
		if(mScheduleLast == NULL)
			mScheduleLast = mScheduleCur;
		//XXX should we backtrack before this in case we have new nodes?
		//advance until we're at the correct location in the schedule:
		//the next node is null or the next time is greater than the current time
		while(mScheduleCur->next != NULL && mScheduleCur->next->time < transport.position())
			mScheduleCur = mScheduleCur->next;

		if(mScheduleLast == mScheduleCur){
			mScheduleLast->command->execute();
		} else {
			//execute all of the stuff we need to execute
			//XXX is this correct, with the timing?
			while(mScheduleLast != mScheduleCur){
				mScheduleLast->command->execute();
				mScheduleLast = mScheduleLast->next;
			}
		}
	}

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
	mScheduleCur = mScheduleLast = NULL;
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
