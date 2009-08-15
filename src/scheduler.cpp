#include "scheduler.hpp"
#include "timepoint.hpp"
#include <iostream>

using std::cerr;
using std::endl;

using namespace DataJockey;

Scheduler::Scheduler() : 
	mCommandsIn(1024), mCommandsOut(1024)
{
}

void Scheduler::execute(Command * cmd){
	if(mCommandsIn.getWriteSpace())
		mCommandsIn.write(cmd);
	else {
		cerr << "no space in command queue, deleting command" << endl;
		delete cmd;
	}
}

void Scheduler::schedule(const TimePoint &time, const Command &cmd){
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
		mCommandsOut.write(cmd);
	}
}

