#ifndef DATAJOCKEY_COMMAND_CPP
#define DATAJOCKEY_COMMAND_CPP

#include "timepoint.hpp"

namespace DataJockey {
	class Command {
		public:
			virtual ~Command();
			const TimePoint& time_executed();
			void time_executed(TimePoint const & t);
			virtual void execute() = 0;
			//this is executed back in the main thread
			//after the command has come back from the audio thread
			//by default it does nothing
			virtual void execute_done();
		private:
			TimePoint mTimeExecuted;
	};
}

/*
#include <string>
#include <map>
#include <boost/pool/detail/singleton.hpp>

namespace DataJockey {
	class CommandFactory : public boost::details::pool::singleton_default<CommandFactory> {
		public:
			typedef std::string identifier_t;
			typedef std::string arg_list_t;
			typedef Command* (*create_command_callback)(arg_list_t);
			bool register_command(const identifier_t& id, create_command_callback cb);
			Command* create_command(identifier_t t, arg_list_t args);
		private:
			typedef std::map<identifier_t, create_command_callback> callback_map_t;
		private:
			callback_map_t mCallbacks;
	};
}
*/

#endif
