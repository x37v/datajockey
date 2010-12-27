#ifndef DATAJOCKEY_AUDIOBUFFER_HPP
#define DATAJOCKEY_AUDIOBUFFER_HPP

#include <vector>
#include <stdexcept>

namespace DataJockey {
   namespace Internal {
      class AudioBuffer {
         public:
            AudioBuffer(std::string soundfileLocation) throw(std::runtime_error);
            //getters
            unsigned int sample_rate();
            unsigned int channels();
            unsigned int length();
            //grab a sample
            float sample(unsigned int channel, unsigned int index);
            float sample(unsigned int channel, unsigned int index, double subsample);
         protected:
            void load(std::string soundfileLocation) throw(std::runtime_error);
         private:
            std::vector<std::vector<float> > mAudioData;
            unsigned int mSampleRate;
      };
   }
}

#endif
