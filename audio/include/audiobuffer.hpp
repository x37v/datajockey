#ifndef DATAJOCKEY_AUDIOBUFFER_HPP
#define DATAJOCKEY_AUDIOBUFFER_HPP

#include <vector>
#include <stdexcept>
#include "soundfile.hpp"

namespace DataJockey {
   namespace Audio {
      class AudioBuffer {
         public:
            typedef std::vector<std::vector<float> > data_buffer_t; 
            typedef void (* progress_callback_t)(int percent, void * user_data);

            AudioBuffer(std::string soundfileLocation) throw(std::runtime_error);
            //returns true if completely loaded
            bool load(progress_callback_t progress_callback = NULL, void * user_data = NULL);
            void abort_load();

            //getters
            unsigned int sample_rate();
            unsigned int channels();
            unsigned int length();
            bool loaded();
            //grab a sample
            float sample(unsigned int channel, unsigned int index);
            float sample(unsigned int channel, unsigned int index, double subsample);

            //get the data buffer
            const data_buffer_t& raw_buffer();
         private:
            SoundFile mSoundFile;
            data_buffer_t mAudioData;
            unsigned int mSampleRate;
            bool mLoaded;
            bool mAbort;
      };
   }
}

#endif
