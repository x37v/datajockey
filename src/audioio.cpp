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
#include "audioio.hpp"

using namespace DataJockey;

AudioIO::AudioIO() : 
	JackCpp::AudioIO("datajockey", 0, 0){
		addOutPort("output0");
		addOutPort("output1");
		addOutPort("cue0");
		addOutPort("cue1");
		mMaster = Master::instance();
}

Master * AudioIO::master(){
	return mMaster;
}

void AudioIO::start(){
	mMaster->setup_audio(getSampleRate(), getBufferSize());
	JackCpp::AudioIO::start();
}

int AudioIO::audioCallback(jack_nframes_t nframes, 
		// A vector of pointers to each input port.
		audioBufVector inBufs,
		// A vector of pointers to each output port.
		audioBufVector outBufs){
	mMaster->audio_compute_and_fill(outBufs, nframes);
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
