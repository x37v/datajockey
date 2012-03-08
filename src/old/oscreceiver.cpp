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
#include "crossfademodel.hpp"
#include "mastermodel.hpp"
#include "mixerpanelmodel.hpp"
#include "djmixerchannelmodel.hpp"
#include "djmixercontrolmodel.hpp"
#include "mixerchannelmodel.hpp"
#include "eqmodel.hpp"
#include <stdlib.h>
#include <iostream>

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

OscReceiver::OscReceiver(MixerPanelModel * model){
	mModel = model;
}


void OscReceiver::ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName&  ){
	boost::regex top_re("^/datajockey/(\\w*)(.*)$");
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
		if(mixer >= mModel->numMixerChannels())
			return;
		if(boost::regex_match(remain.c_str(), matches, volume_re)){
			//make sure our matches list is long enough and that we have an argument
			if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
				float num = floatFromOscNumber(*arg_it);
				//"" == absolute, otherwise, relative
				if(strcmp("", matches[1].str().c_str()) == 0)
					mModel->mixerChannels()->at(mixer)->setVolume(num);
				else {
					mModel->mixerChannels()->at(mixer)->setVolume(
							mModel->mixerChannels()->at(mixer)->volume() + num);
				}
			} else
				throw osc::MissingArgumentException();
		} else if(boost::regex_match(remain.c_str(), matches, mute_re)){
			//make sure our matches list is long enough to test
			if(matches.size() == 2){
				//if we have no argument then we're just setting the mute
				//otherwise toggle mute
				if(strcmp("", matches[1].str().c_str()) == 0) {
					if(arg_it != m.ArgumentsEnd())
						mModel->mixerChannels()->at(mixer)->setMuted(boolFromBoolOrInt(*arg_it));
					else 
						throw osc::MissingArgumentException();
				} else {
					mModel->mixerChannels()->at(mixer)->setMuted(
							!mModel->mixerChannels()->at(mixer)->muted());
				}
			}
		} else if(boost::regex_match(remain.c_str(), matches, eq_re)){
			if(matches.size() == 3){
				EQModel * eqModel = mModel->mixerChannels()->at(mixer)->eq();
				//figure out the band
				EQModel::band band;
				if(strcmp(matches[1].str().c_str(), "low") == 0)
					band = EQModel::LOW;
				else if(strcmp(matches[1].str().c_str(), "mid") == 0)
					band = EQModel::MID;
				else
					band = EQModel::HIGH;

				if(strcmp(matches[2].str().c_str(), "") == 0){
					//absolute
					if(arg_it == m.ArgumentsEnd())
						throw osc::MissingArgumentException();
					else
						eqModel->set(band, floatFromOscNumber(*arg_it));
				} else if(strcmp(matches[2].str().c_str(), "/cut") == 0){
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
			}
		} else if(boost::regex_match(remain.c_str(), load_re)){
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			int work = intFromOsc(*arg_it);
			mModel->mixerChannels()->at(mixer)->loadWork(work);
			//otherwise it is a djmixer control message [or not valid]
		} else {
			processDJControlMessage(remain.c_str(), mModel->mixerChannels()->at(mixer)->control(), m);
		}
	}
}

void OscReceiver::processDJControlMessage(const std::string addr, 
		DJMixerControlModel * control, 
		const osc::ReceivedMessage& m){
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
		//"" == set else toggle
		if(strcmp(matches[1].str().c_str(), "") == 0){
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			else
				control->setPlay(boolFromBoolOrInt(*arg_it));
		} else 
			control->setPlay(!control->playing());
	} else if(boost::regex_match(addr.c_str(), matches, reset_re)){
			control->resetWorkPosition();
	} else if(boost::regex_match(addr.c_str(), matches, cue_re)){
		if(strcmp(matches[1].str().c_str(), "") == 0){
			//set
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			else
				control->setCueing(boolFromBoolOrInt(*arg_it));
		} else 
			control->setCueing(!control->cueing());
	} else if(boost::regex_match(addr.c_str(), matches, sync_re)){
		if(strcmp(matches[1].str().c_str(), "") == 0){
			//set
			if(arg_it == m.ArgumentsEnd())
				throw osc::MissingArgumentException();
			else
				control->setSync(boolFromBoolOrInt(*arg_it));
		} else 
			control->setSync(!control->synced());
	} else if(boost::regex_match(addr.c_str(), matches, seek_re)){
		if(arg_it == m.ArgumentsEnd())
			throw osc::MissingArgumentException();
		int arg = intFromOsc(*arg_it);
		if(strcmp(matches[1].str().c_str(), "") == 0){
			control->setPlaybackPosition(arg);
		} else 
			control->seek(arg);
	} else if(boost::regex_match(addr.c_str(), matches, beatoffset_re)){
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
		float arg = floatFromOscNumber(*arg_it);
		if(strcmp("", matches[1].str().c_str()) == 0)
			mModel->crossFade()->setPosition(arg);
		else
			mModel->crossFade()->setPosition(mModel->crossFade()->position() + arg);
	} else if(boost::regex_match(addr.c_str(), mixers_re)){
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
			float num = floatFromOscNumber(*arg_it);
			//"" == absolute, otherwise, relative
			if(strcmp("", matches[1].str().c_str()) == 0)
				mModel->master()->setVolume(num);
			else 
				mModel->master()->setVolume(mModel->master()->volume() + num);
		} else
			throw osc::MissingArgumentException();
	} else if(boost::regex_match(addr.c_str(), matches, tempo_re)){
		//make sure our matches list is long enough and that we have an argument
		if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
			float num = floatFromOscNumber(*arg_it);
			//"" == absolute, otherwise, relative
			if(strcmp("", matches[1].str().c_str()) == 0)
				mModel->master()->setTempo(num);
			else 
				mModel->master()->setTempo(mModel->master()->tempo() + num);
		} else
			throw osc::MissingArgumentException();
	} else if(boost::regex_match(addr.c_str(), sync_re)){
		//make sure our matches list is long enough and that we have an argument
		if(matches.size() == 2 && arg_it != m.ArgumentsEnd()){
			int src = intFromOsc(*arg_it);
			mModel->master()->setSyncSource(src);
		} else
			throw osc::MissingArgumentException();
	} else {
		//XXX throw an error?
	}
}

