#ifndef MIDIROUTER_H
#define MIDIROUTER_H

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include "audioio.hpp"

/*
 * note:
 *   on/off -> shift
 *   on -> trigger
 *   off -> trigger
 * cc:
 *   0/non-zero -> shift
 *   non-zero -> trigger or Int or Double
 *
 */

class MidiMap;
class MidiRouter : public QObject {
  Q_OBJECT
  public:
    explicit MidiRouter(djaudio::AudioIO::midi_ringbuff_t *ringbuf, QObject *parent = 0);
  signals:
    void playerValueChangedDouble(int player, QString name, double v);
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);
    void playerTriggered(int player, QString name);

    void masterValueChangedDouble(QString name, double v);
    void masterValueChangedInt(QString name, int v);
    void masterValueChangedBool(QString name, bool v);
    void masterTriggered(QString name);

    void mappingError(QString message);

  public slots:
    void clear();
    void readFile(QString fileName);
    void process(); //grab midi and process it
  private:
    QList<QSharedPointer<MidiMap> > mMappings;
    djaudio::AudioIO::midi_ringbuff_t * mInputRingBuffer;
};

#endif // MIDIROUTER_H
