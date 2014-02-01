#ifndef WORK_FILTER_MODEL_COLLECTION_HPP
#define WORK_FILTER_MODEL_COLLECTION_HPP

#include <QString>
#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>
#include "db.h"

class WorkFilterModel;
class QTimer;

class WorkFilterModelCollection : public QObject {
  Q_OBJECT
  public:
    WorkFilterModelCollection(DB * db, QObject * parent = NULL);
    WorkFilterModel * newFilterModel(QObject * parent = NULL);

  public slots:
    void masterSetValueDouble(QString name, double value);
    void updateHistory(int work_id, QDateTime played_at); //just a relay

  signals:
    void workSelected(int work);
    void currentBPMChanged(double bpm);
    void updatedHistory(int work_id, QDateTime played_at);

  protected slots:
    void bpmSendTimeout();

  private:
    double mCurrentBPM;
    double mLastBPM;
    QTimer * mBPMTimeout;
    DB * mDB;
};

#endif
