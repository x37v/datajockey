/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "oscreceiver.hpp"
#include "osc/OscReceivedElements.h"
#include <boost/regex.hpp>
#include <stdlib.h>
#include <iostream>

#include "audiomodel.hpp"
#include "defines.hpp"

bool boolFromBoolOrInt(const osc::ReceivedMessageArgument a){
	if(a.IsBool())
		return a.AsBool();
	else if(a.IsInt32())
		return (a.AsInt32() != 0);
	else if(a.IsInt64())
		return (a.AsInt64() != 0);
	else
		throw osc::WrongArgumentTypeException();
}

int intFromOsc(const osc::ReceivedMessageArgument a){
	if(a.IsInt32())
		return a.AsInt32();
	else if(a.IsInt64())
		return a.AsInt64();
	else
		throw osc::WrongArgumentTypeException();
}

float floatFromOscNumber(const osc::ReceivedMessageArgument a){
	if(a.IsFloat())
		return a.AsFloat();
	if(a.IsDouble())
		return (float)a.AsDouble();
	else if(a.IsInt32())
		return (float)a.AsInt32();
	else if(a.IsInt64())
		return (float)a.AsInt64();
	else
		throw osc::WrongArgumentTypeException();
}

OscReceiver::OscReceiver(){
   mModel = DataJockey::Audio::AudioModel::instance();
}


void OscReceiver::ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName&  ){
	boost::regex top_re("^/dj/(\\w*)(.*)$");
	boost::regex mixer_re("^mixer$");
	boost::regex xfade_re("^crossfade$");
	boost::regex master_re("^master$");
	boost::cmatch matches;
	std::string addr;
	try {
		if(boost::regex_match(m.AddressPattern(), matches, top_re)){
			std::string sub_match(matches[1]);
			if(boost::regex_match(sub_match, mixer_re)){
				processMixerMessage(matches[2], m);
			} else if(boost::regex_match(sub_match, master_re)){
				processMasterMessage(matches[2], m);
			} else if(boost::regex_match(sub_match, xfade_re)){
				processXFadeMessage(matches[2], m);
			}
		} 
	} catch( osc::Exception& e ){
		std::cerr << "An Exception occured while processing incoming OSC packets." << std::endl;
		std::cerr << e.what() << std::endl;
	}
}

void OscReceiver::processMixerMessage(const std::string addr, const osc::ReceivedMessage& m){
	boost::regex mixer_re("^/(\\d+)/(.+)");
	boost::regex volume_re("^volume(/relative){0,1}/{0,1}$");
	boost::regex mute_re("^mute(/toggle){0,1}/{0,1}$");
	boost::regex eq_re("^eq/(high|mid|low)(/relative|/cut|/cut/toggle){0,1}/{0,1}$");
	boost::regex load_re("^load/{0,1}$");
	boost::cmatch matches;
	osc::ReceivedMessage::const_iterator arg_it = m.ArgumentsBegin();

	if(boost::regex_match(addr.c_str(), matches, mixer_re)){
		unsigned int mixer = (unsigned int)atoi(matches[1].str().c_str());
		std::string remain(matches[2].str());
		//make sure we're in range
		if(mixer >= mModel->player_count())
			return;
		if(boost::regex_match(remain.c_str(), matches, volume_re)){
			//make sure our matches list is long enough and that we have an argument
			if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
            int vol = floatFromOscNumber(*arg_it) * (float)DataJockey::one_scale;
            //"" == absolute, otherwise, relative
            if(strcmp("", matches[1].str().c_str()) != 0)
               vol += mModel->player_volume(mixer);
            player_set(mixer, "volume", vol);
			} else
				throw osc::MissingArgumentException();
		} 
      else if(boost::regex_match(remain.c_str(), matches, mute_re)){
			//make sure our matches list is long enough to test
			if(matches.size() == 2){
            bool mute;
				//if we have no argument then we're just setting the mute
				//otherwise toggle mute
				if(strcmp("", matches[1].str().c_str()) == 0) {
					if(arg_it != m.ArgumentsEnd())
						mute = boolFromBoolOrInt(*arg_it);
					else 
						throw osc::MissingArgumentException();
				} else {
               mute = !mModel->player_mute(mixer);
				}
            player_set(mixer, "mute", mute);
			}
		} else if(boost::regex_match(remain.c_str(), matches, eq_re)){
			if(matches.size() == 3){
            int band = 0;
				if(strcmp(matches[1].str().c_str(), "mid") == 0)
               band = 1;
				else if(strcmp(matches[1].str().c_str(), "high") == 0)
               band = 2;

				if(strcmp(matches[2].str().c_str(), "") == 0){
					//absolute
					if(arg_it == m.ArgumentsEnd())
						throw osc::MissingArgumentException();
               int val = floatFromOscNumber(*arg_it) * (float)DataJockey::one_scale;
               QString sband = "eq_high";
               switch(band) {
                  case 0:
                     sband = "eq_low";
                     break;
                  case 1:
                     sband = "eq_mid";
                     break;
               }
               player_set(mixer, sband, val);
            } 
#if 0
            else if(strcmp(matches[2].str().c_str(), "/cut") == 0){
					//cut
					if(arg_it == m.ArgumentsEnd())
						throw osc::MissingArgumentException();
					else
						eqModel->cut(band, boolFromBoolOrInt(*arg_it));
				} else if(strcmp(matches[2].str().c_str(), "/cut/toggle") == 0){
					//toggle cut
					eqModel->toggleCut(band);
				} else {
					//otherwise it is relative
					if(arg_it == m.ArgumentsEnd())
						throw osc::MissingArgumentException();
					else {
						eqModel->set(band, 
								eqModel->value(band) + floatFromOscNumber(*arg_it));
					}
				}
#endif
			}
		} else if(boost::regex_match(remain.c_str(), load_re)){
         /*
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			int work = intFromOsc(*arg_it);
			mModel->mixerChannels()->at(mixer)->loadWork(work);
         */
			//otherwise it is a djmixer control message [or not valid]
		} else {
			processDJControlMessage(remain.c_str(), mixer, m);
		}
	}
}

void OscReceiver::processDJControlMessage(const std::string addr, int mixer, const osc::ReceivedMessage& m){
	boost::regex play_re("^play(/toggle){0,1}/{0,1}$");
	boost::regex cue_re("^cue(/toggle){0,1}/{0,1}$");
	boost::regex sync_re("^sync(/toggle){0,1}/{0,1}$");
	boost::regex seek_re("^seek(/relative){0,1}/{0,1}$");
	boost::regex reset_re("^reset$");
	boost::regex beatoffset_re("^beatoffset(/relative){0,1}/{0,1}$");
	boost::regex tempomul_re("^tempomul/{0,1}$");
	boost::cmatch matches;
	osc::ReceivedMessage::const_iterator arg_it = m.ArgumentsBegin();

	if(boost::regex_match(addr.c_str(), matches, play_re)){
      bool pause;
		//"" == set else toggle
		if(strcmp(matches[1].str().c_str(), "") == 0){
         if(arg_it == m.ArgumentsEnd())
            throw osc::MissingArgumentException();
         else
            pause = !boolFromBoolOrInt(*arg_it);
		} else {
         pause = !mModel->player_pause(mixer);
      }
      player_set(mixer, "pause", pause);
	} else if(boost::regex_match(addr.c_str(), matches, reset_re)){
      QMetaObject::invokeMethod(mModel, "set_player_position", Qt::QueuedConnection,
            Q_ARG(int, mixer),
            Q_ARG(double, 0.0));
	} else if(boost::regex_match(addr.c_str(), matches, cue_re)){
      bool cue;
		if(strcmp(matches[1].str().c_str(), "") == 0){
			//set
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			else
				cue = boolFromBoolOrInt(*arg_it);
		} else 
         cue = !mModel->player_cue(mixer);
      player_set(mixer, "cue", cue);
	} else if(boost::regex_match(addr.c_str(), matches, sync_re)){
      bool sync;
		if(strcmp(matches[1].str().c_str(), "") == 0){
			//set
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			else
				sync = boolFromBoolOrInt(*arg_it);
		} else 
         sync = !mModel->player_sync(mixer);
      player_set(mixer, "sync", sync);
	} else if(boost::regex_match(addr.c_str(), matches, seek_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int beats = intFromOsc(*arg_it);

      //absolute
		if(strcmp(matches[1].str().c_str(), "") == 0){
			//control->setPlaybackPosition(arg);
		} else {
         player_set(mixer, "seek_beat_relative", beats);
      }
	} 
#if 0
   else if(boost::regex_match(addr.c_str(), matches, beatoffset_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int arg = intFromOsc(*arg_it);
		if(strcmp(matches[1].str().c_str(), "") == 0){
			control->setBeatOffset(arg);
		} else 
			control->setBeatOffset(control->beatOffset() + arg);
	} else if(boost::regex_match(addr.c_str(), tempomul_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		float mul = floatFromOscNumber(*arg_it);
		control->setTempoMul(mul);
	} else {
		//XXX throw an error?
	}
#endif
}

void OscReceiver::processXFadeMessage(const std::string addr, const osc::ReceivedMessage& m){
	boost::regex position_re("^(/relative){0,1}/{0,1}$");
	//boost::regex leftmixer_re("^/mixer/left/{0,1}$");
	//boost::regex rightmixer_re("^/mixer/right/{0,1}$");
	boost::regex mixers_re("^/mixers/{0,1}$");
	boost::regex enable_re("^/enable/{0,1}$");
	boost::cmatch matches;
	osc::ReceivedMessage::const_iterator arg_it = m.ArgumentsBegin();

	if(boost::regex_match(addr.c_str(), matches, position_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int pos = (float)DataJockey::one_scale * floatFromOscNumber(*arg_it);
		if(strcmp("", matches[1].str().c_str()) == 0) {
         QMetaObject::invokeMethod(mModel, "set_master_cross_fade_position", Qt::QueuedConnection,
               Q_ARG(int, pos));
      }
		//else
			//mModel->crossFade()->setPosition(mModel->crossFade()->position() + arg);
	} 
#if 0
   else if(boost::regex_match(addr.c_str(), mixers_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int left = intFromOsc(*arg_it);
		arg_it++;
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int right = intFromOsc(*arg_it);
		mModel->crossFade()->setMixers(left, right);
		/*
	} else if(boost::regex_match(addr.c_str(), leftmixer_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int arg = intFromOsc(*arg_it);
		mModel->crossFade()->setLeftMixer(arg);
	} else if(boost::regex_match(addr.c_str(), rightmixer_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int arg = intFromOsc(*arg_it);
		mModel->crossFade()->setRightMixer(arg);
		*/
	} else if(boost::regex_match(addr.c_str(), enable_re)){
		if(arg_it == m.ArgumentsEnd())
			mModel->crossFade()->enable();
		else
			mModel->crossFade()->enable(boolFromBoolOrInt(*arg_it));
	}
#endif
}

void OscReceiver::processMasterMessage(const std::string addr, const osc::ReceivedMessage& m){
	boost::regex volume_re("^/volume(/relative){0,1}/{0,1}$");
	boost::regex tempo_re("^/tempo(/relative){0,1}/{0,1}$");
	boost::regex sync_re("^/syncsource/{0,1}$");
	boost::cmatch matches;
	osc::ReceivedMessage::const_iterator arg_it = m.ArgumentsBegin();
	if(boost::regex_match(addr.c_str(), matches, volume_re)){
		//make sure our matches list is long enough and that we have an argument
		if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
			int vol = floatFromOscNumber(*arg_it) * (float)DataJockey::one_scale;
			//"" == absolute, otherwise, relative
			if(strcmp("", matches[1].str().c_str()) == 0) {
            QMetaObject::invokeMethod(mModel, "set_master_volume", Qt::QueuedConnection,
                  Q_ARG(int, vol));
         }
			//else 
				//mModel->master()->setVolume(mModel->master()->volume() + num);
		} else
			throw osc::MissingArgumentException();
	} else if(boost::regex_match(addr.c_str(), matches, tempo_re)){
		//make sure our matches list is long enough and that we have an argument
		if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
			float bpm = floatFromOscNumber(*arg_it);
			//"" == absolute, otherwise, relative
			if(strcmp("", matches[1].str().c_str()) != 0)
				bpm += mModel->master_bpm();
         QMetaObject::invokeMethod(mModel, "set_master_bpm", Qt::QueuedConnection,
               Q_ARG(double, bpm));
		} else
			throw osc::MissingArgumentException();
	} else if(boost::regex_match(addr.c_str(), sync_re)){
#if 0
		//make sure our matches list is long enough and that we have an argument
		if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
			int src = intFromOsc(*arg_it);
			mModel->master()->setSyncSource(src);
		} else
			throw osc::MissingArgumentException();
#endif
	} else {
		//XXX throw an error?
	}
}

void OscReceiver::player_trigger(int player_index, QString name) {
   QMetaObject::invokeMethod(mModel, "player_trigger", Qt::QueuedConnection,
         Q_ARG(int, player_index),
         Q_ARG(QString, name));
}

void OscReceiver::player_set(int player_index, QString name, bool value) {
   QMetaObject::invokeMethod(mModel, "set_player", Qt::QueuedConnection,
         Q_ARG(int, player_index),
         Q_ARG(QString, name),
         Q_ARG(bool, value));
}

void OscReceiver::player_set(int player_index, QString name, int value) {
   QMetaObject::invokeMethod(mModel, "set_player", Qt::QueuedConnection,
         Q_ARG(int, player_index),
         Q_ARG(QString, name),
         Q_ARG(int, value));
}

#include "ip/UdpSocket.h"

OscThread::OscThread(unsigned int port) {
	mPort = port;
}

void OscThread::run(){
	UdpListeningReceiveSocket s(
			IpEndpointName( IpEndpointName::ANY_ADDRESS, mPort ),
			&mOscReceiver );
	s.Run();
}
