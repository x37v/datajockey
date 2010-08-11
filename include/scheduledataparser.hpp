#ifndef DATAJOCKEY_SCHEDULE_DATA_PARSER_HPP
#define DATAJOCKEY_SCHEDULE_DATA_PARSER_HPP

#include "types.hpp"
#include "schedulenode.hpp"

namespace DataJockey {
	namespace ScheduleDataParser {
		/**
		 * register_command_parser registers a command parser. 
		 * When the parser is running, it will match input against the "key"
		 * string, and if the string matches, call the parse_function given on
		 * the data associated with that key.  The parse_function should produce
		 * a command object, or null on failure.
		 *
		 * @param key The string that identifies this command parser.  If this
		 * string is matched by the ScheduleDataParser, its corresponding data
		 * will be passed to the parse_function.
		 * @param parse_function The function that will be called to produce a
		 * command from the input data.
		 */
		static void register_command_parser(std::string key, 
				Command *(*parse_function)(const std::string key, const CommandIOData& inputData));

		static bool parse_data(const std::string data, ScheduleNode * result);
	}
}

#endif
