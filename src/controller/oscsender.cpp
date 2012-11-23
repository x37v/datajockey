#include "oscsender.hpp"
#include "config.hpp"

#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>
#include "defines.hpp"

#define OSC_OUTPUT_BUFFER_SIZE 1024

using namespace dj::controller;

namespace {
  typedef osc::OutboundPacketStream packet_stream;
  typedef osc::MessageTerminator msg_end;

  void send(QList<UdpTransmitSocket *>& sockets, const packet_stream& stream) {
    foreach(UdpTransmitSocket * socket, sockets)
      socket->Send(stream.Data(), stream.Size());
  }
  
  osc::BeginMessage player_start(int player_index, QString& name) {
    QString addr("/dj/player/");
    QString idx;
    idx.setNum(player_index);
    addr.append(idx);
    addr.append("/");
    addr.append(name);
    return osc::BeginMessage(addr.toStdString().c_str());
  }

  osc::BeginMessage master_start(QString& name) {
    QString addr("/dj/master/");
    addr.append(name);
    return osc::BeginMessage(addr.toStdString().c_str());
  }
}

OSCSender::OSCSender(QObject * parent) : QThread(parent) {
  //set up the default destinations
  Configuration * config = Configuration::instance();
  foreach (OscNetAddr addr, config->osc_destinations())
    add_destination(addr.first, addr.second);
}

OSCSender::~OSCSender() {
  foreach(UdpTransmitSocket * socket, mDestinations)
    delete socket;
}

void OSCSender::add_destination(QString address, int port) {
  UdpTransmitSocket * new_socket = new UdpTransmitSocket(IpEndpointName(address.toStdString().c_str(), port));
  mDestinations << new_socket;
}

void OSCSender::player_trigger(int player_index, QString name){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << player_start(player_index, name) << msg_end();

  send(mDestinations, p);
}

void OSCSender::player_set(int player_index, QString name, bool value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << player_start(player_index, name) << value << msg_end();


  send(mDestinations, p);
}

void OSCSender::player_set(int player_index, QString name, int value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << player_start(player_index, name) << value << msg_end();

  send(mDestinations, p);
}

void OSCSender::player_set(int player_index, QString name, double value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << player_start(player_index, name) << value << msg_end();

  send(mDestinations, p);
}

void OSCSender::player_set(int /*player_index*/, QString /*name*/, TimePoint /*value*/){
  /*
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << player_start(player_index, name);

  send(mDestinations, p);
  */
}


void OSCSender::master_trigger(QString name){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << master_start(name) << msg_end();

  send(mDestinations, p);
}

void OSCSender::master_set(QString name, bool value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << master_start(name) << value << msg_end();

  send(mDestinations, p);
}

void OSCSender::master_set(QString name, int value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << master_start(name) << value << msg_end();

  send(mDestinations, p);
}

void OSCSender::master_set(QString name, double value){
  if (name.contains("update"))
    return;
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << master_start(name) << value << msg_end();

  send(mDestinations, p);
}

