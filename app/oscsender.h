#ifndef OSCSENDER_H
#define OSCSENDER_H

#include <QObject>
#include "lo/lo.h"

class OSCSender : public QObject {
  Q_OBJECT
  public:
    OSCSender(QObject * parent = nullptr);
    virtual ~OSCSender();
  public slots:
    //example "osc.udp://localhost:4444/my/path/"
    void setAddress(QString url);
    //empty string for NULL/local
    void setHostAndPort(QString host, QString port);

    void playerSetValueDouble(int player, QString name, double v);
    void playerSetValueInt(int player, QString name, int v);
    void playerSetValueBool(int player, QString name, bool v);
    void playerTrigger(int player, QString name);

    void masterSetValueDouble(QString name, double v);
    void masterSetValueInt(QString name, int v);
    void masterSetValueBool(QString name, bool v);
    void masterTrigger(QString name);
  private:
    lo_address mAddress = nullptr;
};

#endif
