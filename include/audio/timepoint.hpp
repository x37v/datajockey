#ifndef DATAJOCKEY_TIMEPOINT_HPP
#define DATAJOCKEY_TIMEPOINT_HPP

namespace DataJockey {
   namespace Audio {
      class TimePoint {
         public:
            enum time_type {BEAT_BAR, SECONDS};
            TimePoint(time_type t = BEAT_BAR);
            TimePoint(double sec);
            TimePoint(int bar, unsigned int beat, double pos_in_beat = 0.0);
            TimePoint(const TimePoint& other);
            TimePoint& operator=(const TimePoint& other);
            //getters
            time_type type() const;
            int bar() const;
            unsigned int beat() const;
            //this simply tells how far into the beat we are.. 0..1
            double pos_in_beat() const;
            unsigned int beats_per_bar() const;
            unsigned int beat_type() const;
            double seconds() const;
            bool valid() const;
            //setters
            void type(time_type t);
            void bar(int val);
            void beat(unsigned int val);
            void pos_in_beat(double val);
            void beats_per_bar(unsigned int val);
            void beat_type(unsigned int val);
            void seconds(double val);

            //these reset the internal state
            void at_bar(int newBar, unsigned int newBeat = 0, double newPos = 0.0);
            void at_beat(int newBeat, double newPos = 0.0);

            void advance_beat();
            void invalidate();

            //comparison operators
            //XXX should throw error if comparing against invalid or wrong type ?
            bool operator==(const TimePoint &other) const;
            bool operator!=(const TimePoint &other) const;
            bool operator<(const TimePoint &other) const;
            bool operator>(const TimePoint &other) const;
            bool operator<=(const TimePoint &other) const;
            bool operator>=(const TimePoint &other) const;
            const TimePoint operator+(const TimePoint &other) const;
            const TimePoint operator-(const TimePoint &other) const;

            TimePoint& operator+=(const TimePoint &other);
            TimePoint& operator-=(const TimePoint &other);

            //negation
            friend TimePoint operator-(const TimePoint &timepoint);
         private:
            time_type mType;
            //mirroring jack transport
            int mBar;
            unsigned int mBeat;
            double mPosInBeat;
            unsigned int mBeatsPerBar; //time signature "numerator"
            unsigned int mBeatType; //time signature "denominator"
            double mSeconds;
      };
   }
}

#endif
