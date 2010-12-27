#ifndef DATAJOCKEY_AUDIOIO_HPP
#define DATAJOCKEY_AUDIOIO_HPP

#include "jackaudioio.hpp"
#include "master.hpp"
#include <vector>

namespace DataJockey {
   namespace Internal {
      class AudioIO : public JackCpp::AudioIO {
         private:
            AudioIO();
            ~AudioIO();
            AudioIO(const AudioIO&);
            AudioIO& operator=(const AudioIO&);
            static AudioIO * cInstance;
         public:
            static AudioIO * instance();
            Master * master();
            void start();
         protected:
            virtual int audioCallback(
                  jack_nframes_t nframes, 
                  audioBufVector inBufs,
                  audioBufVector outBufs);
            Master * mMaster;
      };
   }
}

#endif
