#ifndef DATAJOCKEY_COMMAND_HPP
#define DATAJOCKEY_COMMAND_HPP

#include "timepoint.hpp"
#include "types.hpp"

namespace DataJockey {
   namespace Audio {
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
            //this is called in order to populate a CommandIOData object from this
            //Command object
            //return true on actual storage, false on empty
            virtual bool store(CommandIOData& data) const = 0;
         private:
            TimePoint mTimeExecuted;
      };
   }
}

#endif
