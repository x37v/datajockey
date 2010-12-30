#ifndef DATAJOCKEY_AUDIOBUFFER_HPP
#define DATAJOCKEY_AUDIOBUFFER_HPP

#include <vector>
#include <stdexcept>
#include "soundfile.hpp"

namespace DataJockey {
   class AudioBuffer {
      public:
         typedef void (* progress_callback_t)(int percent, void * user_data);

         AudioBuffer(std::string soundfileLocation) throw(std::runtime_error);
         void load(progress_callback_t progress_callback = NULL, void * user_data = NULL);

         //getters
         unsigned int sample_rate();
         unsigned int channels();
         unsigned int length();
         bool loaded();
         //grab a sample
         float sample(unsigned int channel, unsigned int index);
         float sample(unsigned int channel, unsigned int index, double subsample);
      private:
         SoundFile mSoundFile;
         std::vector<std::vector<float> > mAudioData;
         unsigned int mSampleRate;
         bool mLoaded;
   };
}

#endif
