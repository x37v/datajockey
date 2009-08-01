#ifndef DATAJOCKEY_TIMEPOINT_HPP
#define DATAJOCKEY_TIMEPOINT_HPP

namespace DataJockey {
	class TimePoint {
		public:
			enum time_type {BEAT_BAR, SECONDS};
			TimePoint();
			//getters
			time_type type() const;
			int bar() const;
			int beat() const;
			//this simply tells how far into the beat we are.. 0..1
			double pos_in_beat() const;
			float beats_per_bar() const;
			float beat_type() const;
			double seconds() const;
			bool valid() const;
			//setters
			void type(time_type t);
			void bar(int val);
			void beat(int val);
			void pos_in_beat(double val);
			void beats_per_bar(float val);
			void beat_type(float val);
			void seconds(double val);
			
			void invalidate();
		private:
			time_type mType;
			//mirroring jack transport
			int mBar;
			int mBeat;
			double mPosInBeat;
			float mBeatsPerBar;	//time signature "numerator"
			float	mBeatType; //time signature "denominator"
			double mSeconds;
	};
}

#endif
