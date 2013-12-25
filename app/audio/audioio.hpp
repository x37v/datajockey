#ifndef DATAJOCKEY_AUDIOIO_HPP
#define DATAJOCKEY_AUDIOIO_HPP

#include "jackaudioio.hpp"
#include "jackmidiport.hpp"
#include "jackringbuffer.hpp"
#include "master.hpp"
#include <vector>

namespace djaudio {
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
      void run(bool doit);
      struct midi_event_buffer_t { uint8_t data[3]; };
      typedef JackCpp::RingBuffer<midi_event_buffer_t> midi_ringbuff_t;
      midi_ringbuff_t * midi_input_ringbuffer();
    protected:
      virtual int audioCallback(
          jack_nframes_t nframes, 
          audioBufVector inBufs,
          audioBufVector outBufs);
      Master * mMaster;

      JackCpp::MIDIInPort mMIDIIn;
      midi_ringbuff_t mMIDIEventFromAudio;
  };
}

#endif
