#ifndef DATAJOCKEY_BEATBUFFER_HPP
#define DATAJOCKEY_BEATBUFFER_HPP

#include "timepoint.hpp"
#include <string>
#include <stdexcept>
#include <deque>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>
#include <QString>

namespace dj {
   namespace audio {
      //XXX assuming 4/4 for now
      class BeatBuffer : public QSharedData {
         public:
            typedef std::deque<double> beat_list_t;
            typedef beat_list_t::const_iterator const_iterator;
            typedef beat_list_t::iterator iterator;

            BeatBuffer();
            virtual ~BeatBuffer();
            bool load(const QString& file_location);

            double time_at_position(const TimePoint& position) const;
            TimePoint position_at_time(double seconds) const;
            TimePoint beat_closest(double seconds) const;
            unsigned int index(double seconds) const; //which index would we be at at this time
            unsigned int index_closest(double seconds) const;

            //this one lets us give a 'last position' helper so we don't have to
            //search the whole thing
            TimePoint position_at_time(double seconds, const TimePoint& lastPos) const;
            TimePoint end_position() const;
            unsigned int start_offset() const;

            BeatBuffer::const_iterator begin() const;
            BeatBuffer::iterator begin();
            BeatBuffer::const_iterator end() const;
            BeatBuffer::iterator end();

            double at(unsigned int i) const;
            double operator[](unsigned int i) const;

            unsigned int length() const;

            //setters realtime safe
            void start_offset(unsigned int b);

            //update the value of the beat at the index given
            void update_value(unsigned int index, double value);
            void update_value(iterator pos, double value);

            //setters not realtime safe
            void clear();
            void insert_beat(double seconds);

            void smooth(unsigned int iterations);

            //not real time save
            double median_difference();
            void median_and_mean(double& median, double& mean);

         private:
            beat_list_t mBeatData;
      };
      typedef QExplicitlySharedDataPointer<BeatBuffer> BeatBufferPtr;
   }
}

#endif
