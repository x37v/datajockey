#include "command.hpp"
using namespace DataJockey;

Command::~Command(){
}

const TimePoint& Command::time_executed(){ return mTimeExecuted; }
void Command::time_executed(TimePoint const & t){
	mTimeExecuted = t;
}

void Command::execute_done(){
}

/*
bool CommandFactory::register_command(const identifier_t& id, create_command_callback cb){
	return mCallbacks.insert(callback_map_t::value_type(id, cb)).second;
}

Command* CommandFactory::create_command(identifier_t t, arg_list_t args){
	callback_map_t::const_iterator it = mCallbacks.find(t);
	if(it != mCallbacks.end()){
		return (it->second)(args);
	} else
		return NULL;
}

*/
