#include "workfiltermodelcollection.hpp"
#include "workfiltermodel.hpp"
#include "db.h"
#include <QTimer>

namespace {
  const int bpm_timer_timeout_ms = 200;
}

WorkFilterModelCollection::WorkFilterModelCollection(DB * db, QObject * parent) :
  QObject(parent),
  mCurrentBPM(120.0),
  mLastBPM(0.0),
  mDB(db)
{
  //timeout timer
  mBPMTimeout = new QTimer(this);
  mBPMTimeout->setSingleShot(true);
  QObject::connect(mBPMTimeout, SIGNAL(timeout()), SLOT(bpmSendTimeout()));
}

WorkFilterModel * WorkFilterModelCollection::newFilterModel(QObject * parent) {
  WorkFilterModel * m = new WorkFilterModel(mDB, parent);
  m->setCurrentBPM(mCurrentBPM);

  QObject::connect(this, SIGNAL(currentBPMChanged(double)), m, SLOT(setCurrentBPM(double)));
  QObject::connect(this, SIGNAL(updatedHistory(int, QDateTime)), m, SLOT(updateHistory(int, QDateTime)));
  return m;
}

void WorkFilterModelCollection::masterSetValueDouble(QString name, double value){
  if (name == "bpm") {
    if (mCurrentBPM == value)
      return;
    mCurrentBPM = value;
    mBPMTimeout->start(bpm_timer_timeout_ms);
  }
}

void WorkFilterModelCollection::updateHistory(int work_id, QDateTime played_at) {
  emit(updatedHistory(work_id, played_at));
}

void WorkFilterModelCollection::bpmSendTimeout() {
  if (mCurrentBPM != mLastBPM) {
    mLastBPM = mCurrentBPM;
    emit(currentBPMChanged(mCurrentBPM));
  }
}

