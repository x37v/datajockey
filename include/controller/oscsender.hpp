#ifndef DJ_OSCSENDER_HPP
#define DJ_OSCSENDER_HPP

#include <QObject>
#include <QThread>
#include <QList>
#include <QVariant>
#include "timepoint.hpp"

class UdpTransmitSocket;

namespace dj {
  namespace controller {
    using dj::audio::TimePoint;

    class OSCSender : public QThread {
      Q_OBJECT
      public:
        OSCSender(QObject * parent);
        ~OSCSender();
      public slots:
        void add_destination(QString address, int port);

        void player_trigger(int player_index, QString name);
        void player_set(int player_index, QString name, bool value);
        void player_set(int player_index, QString name, int value);
        void player_set(int player_index, QString name, double value);
        void player_set(int player_index, QString name, QString value);
        void player_set(int player_index, QString name, TimePoint value);

        void master_trigger(QString name);
        void master_set(QString name, bool value);
        void master_set(QString name, int value);
        void master_set(QString name, double value);

        void send_quit();
      private:
        QList<UdpTransmitSocket *> mDestinations;
        void player_send(int player_index, QString name, QVariant value);
        void master_send(QString name, QVariant value);
    };
  }
}

#endif
