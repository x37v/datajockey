#ifndef DATAJOCKEY_AUDIOIO_HPP
#define DATAJOCKEY_AUDIOIO_HPP

#include "jackaudioio.hpp"
#include "jackmidiport.hpp"
#include "jackringbuffer.hpp"
#include "master.hpp"
#include <vector>

namespace dj {
   namespace audio {
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
            struct midi_event_buffer_t { uint8_t data[3]; };
         protected:
            virtual int audioCallback(
                  jack_nframes_t nframes, 
                  audioBufVector inBufs,
                  audioBufVector outBufs);
            Master * mMaster;

            JackCpp::MIDIInPort mMIDIIn;
            JackCpp::RingBuffer<midi_event_buffer_t> mMIDIEventFromAudio;
      };
   }
}

#endif
