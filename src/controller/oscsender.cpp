#include "oscsender.hpp"
#include "config.hpp"

#include <osc/OscOutboundPacketStream.h>
#include <ip/UdpSocket.h>
#include "defines.hpp"
#include <iostream>
#include <QDateTime>
#include <stdexcept>

#define OSC_OUTPUT_BUFFER_SIZE 2048

//do we cast booleans integers
#define OSC_BOOL_TO_INT
//do we cast doubles to floats
#define OSC_DOUBLE_TO_FLOAT

using namespace dj::controller;
using std::cerr;
using std::endl;

namespace {
  typedef osc::OutboundPacketStream packet_stream;

  void send(QList<UdpTransmitSocket *>& sockets, const packet_stream& stream) {
    foreach(UdpTransmitSocket * socket, sockets)
      socket->Send(stream.Data(), stream.Size());
  }

  QVariant value_transform(QString& name, QVariant value) {
    if (name.contains("volume") || name.contains("eq_") || name.contains("crossfade_position"))
        return QVariant(static_cast<double>(value.toInt()) / static_cast<double>(dj::one_scale));
    else if (name.contains("speed"))
        return QVariant(static_cast<double>(value.toInt()) / 10.0);
    return value;
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

  void osc_add_value(packet_stream& stream, QString value_name, QVariant value) throw(std::runtime_error) {
    //remap the value if we need
    value = value_transform(value_name, value);

    switch(static_cast<QMetaType::Type>(value.type())) {
      case QMetaType::Bool:
#ifdef OSC_BOOL_TO_INT
        stream << static_cast<int>(value.toBool());
#else
        stream << value.toBool();
#endif
        break;
      case QMetaType::Int:
        stream << value.toInt();
        break;
      case QMetaType::Float:
        stream << value.toFloat();
        break;
      case QMetaType::Double:
#ifdef OSC_DOUBLE_TO_FLOAT
        stream << static_cast<float>(value.toDouble());
#else
        stream << value.toDouble();
#endif
        break;
      case QMetaType::QString:
        stream << value.toString().toStdString().c_str();
        break;
      default:
        throw(std::runtime_error("unsupported osc send type " + std::string(value.typeName())));
    }
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
  p << player_start(player_index, name) << osc::EndMessage;
  //p << osc::TimeTag(QDateTime::currentMSecsSinceEpoch());

  send(mDestinations, p);
}

void OSCSender::player_set(int player_index, QString name, bool value){
  player_send(player_index, name, value);
}

void OSCSender::player_set(int player_index, QString name, int value){
  player_send(player_index, name, value);
}

void OSCSender::player_set(int player_index, QString name, double value){
  player_send(player_index, name, value);
}

void OSCSender::player_set(int player_index, QString name, QString value) {
  player_send(player_index, name, value);
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
  p << master_start(name) << osc::EndMessage;
  //p << osc::TimeTag(QDateTime::currentMSecsSinceEpoch());

  send(mDestinations, p);
}

void OSCSender::master_set(QString name, bool value){
  master_send(name, value);
}

void OSCSender::master_set(QString name, int value){
  master_send(name, value);
}

void OSCSender::master_set(QString name, double value){
  master_send(name, value);
}

void OSCSender::send_quit() {
  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);
  p << osc::BeginMessage("/dj/quit") << osc::EndMessage;
  send(mDestinations, p);
}

void OSCSender::player_send(int player_index, QString name, QVariant value){
  if (name.contains("update"))
    return;

  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);

  p << player_start(player_index, name);
  //p << osc::TimeTag(QDateTime::currentMSecsSinceEpoch());

  try {
    osc_add_value(p, name, value);
  } catch (std::exception& e) {
    cerr << e.what() << endl;
    return;
  }

  p << osc::EndMessage;
  send(mDestinations, p);
}

void OSCSender::master_send(QString name, QVariant value){
  if (name.contains("update"))
    return;

  char b[OSC_OUTPUT_BUFFER_SIZE];
  packet_stream p(b, OSC_OUTPUT_BUFFER_SIZE);

  p << master_start(name);
  //p << osc::TimeTag(QDateTime::currentMSecsSinceEpoch());

  try {
    osc_add_value(p, name, value);
  } catch (std::exception& e) {
    cerr << e.what() << endl;
    return;
  }

  p << osc::EndMessage;
  send(mDestinations, p);
}

