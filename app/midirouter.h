#ifndef MIDIROUTER_H
#define MIDIROUTER_H

#include <QObject>
#include <QList>

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
    explicit MidiRouter(QObject *parent = 0);
  signals:
    void playerValueChangedDouble(int player, QString name, double v);
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);
    void playerTriggered();

    void masterValueChangedDouble(QString name, double v);
    void masterValueChangedInt(QString name, int v);
    void masterValueChangedBool(QString name, bool v);
    void masterTriggered();

  public slots:
    void clear();
    void readFile(QString fileName);
  private:
    QList<MidiMap *> mMappings;
};

#endif // MIDIROUTER_H
