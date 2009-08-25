#ifndef DATAJOCKEY_TRANSPORT_HPP
#define DATAJOCKEY_TRANSPORT_HPP

#include "timepoint.hpp"
#include "command.hpp"

namespace DataJockey {
	class Transport {
		public:
			Transport();
			void setup(unsigned int sampleRate);

			//getters
			const TimePoint& position() const;
			double bpm() const;

			//setters
			void position(const TimePoint &pos);
			void bpm(double val);

			//misc
			//tick the clock, outputs true if new beat, false otherwise
			bool tick();
			//only valid right after tick() 
			double seconds_till_next_beat() const;
			
			//virtual void start() = 0;
			//virtual void stop() = 0;
		private:
			//the current time point of this transport
			TimePoint mPosition;
			double mBPM;
			unsigned int mSampleRate;
			double mIncrement;
			bool mSetup;
			//only valid right after a tick()
			double mSecondsTillNextBeat;
	};

	class TransportCommand : public Command {
		public:
			TransportCommand(Transport * t);
		protected:
			Transport * transport() const;
		private:
			Transport * mTransport;
	};

	class TransportPositionCommand : public TransportCommand {
		public:
			TransportPositionCommand(Transport * t, const TimePoint& pos);
			virtual void execute();
		private:
			TimePoint mTimePoint;
			
	};

	class TransportBPMCommand : public TransportCommand {
		public:
			TransportBPMCommand(Transport * t, double bpm);
			virtual void execute();
		private:
			double mBPM;
	};
}

#endif
