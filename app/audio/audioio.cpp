//an example JACKC++ program
//Copyright 2007 Alex Norman
//
//This file is part of JACKC++.
//
//JACKC++ is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//JACKC++ is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with JACKC++.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <stdlib.h>
#include <jack/transport.h>
#include <cstring>
#include "audioio.hpp"

using namespace djaudio;
using JackCpp::MIDIPort;

AudioIO * AudioIO::cInstance = NULL;

AudioIO::AudioIO() : 
  JackCpp::AudioIO("datajockey", 0, 0),
  mMIDIEventFromAudio(256) //TODO whats a good value for this?
{
  addOutPort("output0");
  addOutPort("output1");
  addOutPort("cue0");
  addOutPort("cue1");
  mMaster = Master::instance();
  mMIDIIn.init(this, "midi_in");
}

AudioIO::~AudioIO(){
}

AudioIO * AudioIO::instance(){
  if(cInstance == NULL)
    cInstance = new AudioIO;
  return cInstance;
}

Master * AudioIO::master(){
  return mMaster;
}

void AudioIO::run(bool doit){
  if (doit) {
    mMaster->setup_audio(getSampleRate(), getBufferSize());
    JackCpp::AudioIO::start();
  } else {
    JackCpp::AudioIO::stop();
  }
}

int AudioIO::audioCallback(jack_nframes_t nframes, 
    // A vector of pointers to each input port.
    audioBufVector /*inBufs*/,
    // A vector of pointers to each output port.
    audioBufVector outBufs){

  //compute the audio 
  mMaster->audio_compute_and_fill(outBufs, nframes);


  //deal with midi
  void * midi_in_buffer = mMIDIIn.port_buffer(nframes);
  const uint32_t midi_in_cnt = mMIDIIn.event_count(midi_in_buffer);

  //push any note or cc events into the ring buffer
  for (uint32_t i = 0; i < midi_in_cnt; i++) {
    jack_midi_event_t evt;
    if (mMIDIIn.get(evt, midi_in_buffer, i)) {
      midi_event_buffer_t buf;
      switch(MIDIPort::status(evt)) {
        case MIDIPort::CC:
        case MIDIPort::NOTEOFF:
        case MIDIPort::NOTEON:
          //if we have space, copy the event data and write it to our buffer
          if (mMIDIEventFromAudio.getWriteSpace()) {
            memcpy(buf.data, evt.buffer, 3);
            mMIDIEventFromAudio.write(buf);
          } else {
            //TODO report error
          }
          break;
        default:
          break;
      }
    }
  }


  /*
     jack_position_t pos;
     jack_transport_state_t state = jack_transport_query (client(), &pos);
     if(state == JackTransportRolling && pos.valid & JackTransportBBT){
     cout << pos.tick << " / " << 
     pos.ticks_per_beat << " = " << 
     (double)pos.tick / pos.ticks_per_beat << endl;
     }
     */
  //return 0 on success
  return 0;
}

AudioIO::midi_ringbuff_t * AudioIO::midi_input_ringbuffer() { return &mMIDIEventFromAudio; }

