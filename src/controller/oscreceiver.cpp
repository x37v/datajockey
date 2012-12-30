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
#include <stdexcept>
#include "audiomodel.hpp"
#include "defines.hpp"
#include <QVariant>

using std::cout;
using std::cerr;
using std::endl;
using dj::audio::AudioModel;

namespace {
  QVariant arg_transform(QString& name, const osc::ReceivedMessageArgument arg) throw(std::runtime_error) {
    if (arg.IsBool())
      return QVariant(arg.AsBool());
    if (arg.IsString())
      return QVariant(QString(arg.AsString()));

    QVariant value;
    if (arg.IsInt32())
      value = QVariant(static_cast<int>(arg.AsInt32()));
    //else if (arg.IsInt64())
      //value = QVariant(static_cast<qlonglong>(arg.AsInt64()));
    else if (arg.IsFloat())
      value = QVariant(static_cast<double>(arg.AsFloat()));
    else if (arg.IsDouble())
      value = QVariant(arg.AsDouble());
    else
      throw std::runtime_error("unsupported osc type: " + arg.TypeTag());

    if (name.contains("volume") || name.contains("eq_") || name.contains("crossfade_position")) {
      if (!value.canConvert(QVariant::Double))
        throw std::runtime_error("cannot convert value of type " + std::string(value.typeName()) + " to double");
      return QVariant(static_cast<int>(dj::one_scale * value.value<double>()));
    }

    //see if we should convert to bool
    QString non_rel = name.remove("_relative");
    if (AudioModel::player_signals["bool"].contains(non_rel) || AudioModel::master_signals["bool"].contains(non_rel))
      return QVariant(value.value<bool>());

    return value;
  }
}

OSCReceiver::OSCReceiver(QObject * parent) : QObject(parent) {
}

OSCReceiver::~OSCReceiver() {
}

void OSCReceiver::ProcessMessage( const osc::ReceivedMessage& m, const IpEndpointName&  /* unused */) {
  boost::regex top_re("^/dj/(\\w*)/(.*)$");
  boost::regex player_re("^player$");
  boost::regex master_re("^master$");
  boost::cmatch matches;
  std::string addr;
  try {
    if(boost::regex_match(m.AddressPattern(), matches, top_re)) {
      std::string sub_match(matches[1]);
      osc::ReceivedMessage::const_iterator arg_it = m.ArgumentsBegin();

      if (boost::regex_match(sub_match, player_re)) {
        sub_match = matches[2].str();
        //pull off the player index
        boost::regex mixer_re("^(\\d+)/(.+)");
        if(!boost::regex_match(sub_match.c_str(), matches, mixer_re))
          return;
        unsigned int player_index = (unsigned int)atoi(matches[1].str().c_str());
        QString command_name = QString::fromStdString(matches[2]).replace('/', "_");

        //if there are no args then send trigger
        if (arg_it == m.ArgumentsEnd()) {
          emit(player_triggered(player_index, command_name));
        } else {
          QVariant value = arg_transform(command_name, *arg_it);
          switch (value.type()) {
            case QMetaType::QString:
              emit(player_value_changed(player_index, command_name, value.value<QString>()));
              break;
            case QMetaType::Bool:
              emit(player_value_changed(player_index, command_name, value.value<bool>()));
              break;
            case QMetaType::Double:
              emit(player_value_changed(player_index, command_name, value.value<double>()));
              break;
            case QMetaType::Int:
              emit(player_value_changed(player_index, command_name, value.value<int>()));
              break;
            default:
              break;
          }
        }

      } else if(boost::regex_match(sub_match, master_re)) {
        QString command_name = QString::fromStdString(matches[2]).replace('/', "_");

        //if there are no args then send trigger
        if (arg_it == m.ArgumentsEnd()) {
          emit(master_triggered(command_name));
        } else {
          QVariant value = arg_transform(command_name, *arg_it);
          switch (value.type()) {
            /*
            case QMetaType::QString:
              emit(master_value_changed(command_name, value.value<QString>()));
              break;
              */
            case QMetaType::Bool:
              emit(master_value_changed(command_name, value.value<bool>()));
              break;
            case QMetaType::Double:
              emit(master_value_changed(command_name, value.value<double>()));
              break;
            case QMetaType::Int:
              emit(master_value_changed(command_name, value.value<int>()));
              break;
            default:
              break;
          }
        }
      } else {
        cerr << m.AddressPattern() << " not supported by datajockey" << endl;
      }
    } 
  } catch(std::exception& e) {
    cerr << "An Exception occured while processing incoming OSC packets." << endl;
    cerr << e.what() << endl;
  } catch(...){
    cerr << "An Exception occured while processing incoming OSC packets." << endl;
  }
}

#include "ip/UdpSocket.h"

OscThread::OscThread(unsigned int port) {
  mPort = port;
}

void OscThread::run(){
  UdpListeningReceiveSocket s(
      IpEndpointName( IpEndpointName::ANY_ADDRESS, mPort ),
      &mOSCReceiver );
  s.Run();
}
