#ifndef DATAJOCKEY_AUDIOBUFFER_HPP
#define DATAJOCKEY_AUDIOBUFFER_HPP

#include <stdexcept>
#include <vector>
#include "soundfile.hpp"

namespace DataJockey {
   namespace Audio {
      class AudioBuffer {
         public:
            typedef std::vector<float > data_buffer_t; 
            typedef void (* progress_callback_t)(int percent, void * user_data);

            AudioBuffer(std::string soundfileLocation) throw(std::runtime_error);
            //returns true if completely loaded
            bool load(progress_callback_t progress_callback = NULL, void * user_data = NULL);
            void abort_load();

            //getters
            unsigned int sample_rate() const;
            unsigned int channels() const;
            unsigned int length() const;
            bool loaded() const;
            //grab a sample
            float sample(unsigned int channel, unsigned int index) const;
            float sample(unsigned int channel, unsigned int index, double subsample) const;

            bool valid() const;

            //get the data buffer
            const data_buffer_t& raw_buffer() const;
         private:
            SoundFile mSoundFile;
            data_buffer_t mAudioData;
            unsigned int mSampleRate;
            bool mLoaded;
            bool mAbort;
            unsigned int mNumChannels;
      };
   }
}

#endif
