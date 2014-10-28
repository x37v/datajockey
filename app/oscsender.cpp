#include "oscsender.h"
#include "defines.hpp"

namespace {
  bool convertToDouble(QString name) {
    return (name == "volume" || name.contains("eq_"));
  }
}

OSCSender::OSCSender(QObject * parent) : QObject(parent) {
}

OSCSender::~OSCSender() {
  if (mAddress)
    lo_address_free(mAddress);
}

void OSCSender::setAddress(QString url) {
  if (mAddress)
    lo_address_free(mAddress);
  mAddress = lo_address_new_from_url(qPrintable(url));
}

void OSCSender::setHostAndPort(QString host, QString port) {
  if (mAddress)
    lo_address_free(mAddress);
  mAddress = lo_address_new(host.length() ? qPrintable(host) : NULL, qPrintable(port));
}

void OSCSender::playerSetValueDouble(int player, QString name, double v) {
  if (!mAddress || name == "audio_level") { return; }
  QString address = QString("/player/%1/%2").arg(player).arg(name);
  lo_send(mAddress, qPrintable(address), "f", static_cast<float>(v));
}

void OSCSender::playerSetValueInt(int player, QString name, int v) {
  if (!mAddress || name == "position_frame") { return; }
  if (convertToDouble(name)) {
    playerSetValueDouble(player, name, dj::to_double(v));
    return;
  }
  QString address = QString("/player/%1/%2").arg(player).arg(name);
  lo_send(mAddress, qPrintable(address), "i", v);
}

void OSCSender::playerSetValueBool(int player, QString name, bool v) {
  if (!mAddress) { return; }
  QString address = QString("/player/%1/%2").arg(player).arg(name);
  lo_send(mAddress, qPrintable(address), "i", v ? 1 : 0);
}

void OSCSender::playerTrigger(int player, QString name) {
  if (!mAddress) { return; }
  QString address = QString("/player/%1/trigger").arg(player);
  lo_send(mAddress, qPrintable(address), "s", qPrintable(name));
}


void OSCSender::masterSetValueDouble(QString name, double v) {
  if (!mAddress || name == "audio_level") { return; }
  QString address = QString("/master/%1").arg(name);
  lo_send(mAddress, qPrintable(address), "f", static_cast<float>(v));
}

void OSCSender::masterSetValueInt(QString name, int v) {
  if (!mAddress) { return; }
  if (convertToDouble(name)) {
    masterSetValueDouble(name, dj::to_double(v));
    return;
  }
  QString address = QString("/master/%1").arg(name);
  lo_send(mAddress, qPrintable(address), "i", v);
}

void OSCSender::masterSetValueBool(QString name, bool v) {
  if (!mAddress) { return; }
  QString address = QString("/master/%1").arg(name);
  lo_send(mAddress, qPrintable(address), "i", v ? 1 : 0);
}

void OSCSender::masterTrigger(QString name) {
  if (!mAddress) { return; }
  lo_send(mAddress, "/master/trigger", "s", qPrintable(name));
}

