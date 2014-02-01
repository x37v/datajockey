#ifndef MIDIROUTER_H
#define MIDIROUTER_H

#include <QObject>

class MidiRouter : public QObject
{
    Q_OBJECT
  public:
    explicit MidiRouter(QObject *parent = 0);

  signals:

  public slots:

};

#endif // MIDIROUTER_H
