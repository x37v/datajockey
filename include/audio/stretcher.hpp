#ifndef STRETCHER_HPP
#define STRETCHER_HPP

#include "audiobuffer.hpp"

namespace dj {
   namespace audio {
      class Stretcher {
         public:
            Stretcher();
            virtual ~Stretcher();

            void audio_buffer(AudioBuffer * buffer);
            AudioBuffer * audio_buffer() const;

            void reset();

            //set the frame
            void frame(unsigned int frame, double frame_subsample = 0.0);
            unsigned int frame() const;

            //set the playback speed
            void speed(double play_speed);
            double speed() const;

            void next_frame(float * frame); 

            virtual bool pitch_independent() const;

         protected:
            virtual void audio_changed();
            virtual void frame_updated();
            virtual void speed_updated();
            virtual void compute_frame(float * frame, unsigned int new_index, double new_index_subsample, unsigned int last_index, double last_index_subsample) = 0;

         private:
            unsigned int mFrame;
            double mFrameSubsample;
            double mSpeed;
            AudioBuffer * mAudioBuffer;
      };
   }
}

#endif
