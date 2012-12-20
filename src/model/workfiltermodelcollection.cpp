#include "workfiltermodelcollection.hpp"
#include "workfiltermodel.hpp"
#include "db.hpp"
#include <QTimer>

using namespace dj;

namespace {
  const int bpm_timer_timeout_ms = 200;
}

WorkFilterModelCollection::WorkFilterModelCollection(QObject * parent, QSqlDatabase db) :
  QObject(parent),
  mCurrentBPM(120.0),
  mLastBPM(0.0)
{
  //timeout timer
  mBPMTimeout = new QTimer(this);
  mBPMTimeout->setSingleShot(true);
  QObject::connect(mBPMTimeout, SIGNAL(timeout()), SLOT(bpm_send_timeout()));
}

WorkFilterModel * WorkFilterModelCollection::new_filter_model() {
  WorkFilterModel * m = new WorkFilterModel(this, model::db::get());
  mFilterModels << m;
  m->set_current_bpm(mCurrentBPM);

  QObject::connect(this, SIGNAL(current_bpm_changed(double)), m, SLOT(set_current_bpm(double)));
  QObject::connect(this, SIGNAL(history_updated()), m, SLOT(update_history()));
  return m;
}

void WorkFilterModelCollection::player_trigger(int player_index, QString name){
}

void WorkFilterModelCollection::player_set(int player_index, QString name, bool value){
}

void WorkFilterModelCollection::player_set(int player_index, QString name, int value){
}

void WorkFilterModelCollection::player_set(int player_index, QString name, double value){
}

void WorkFilterModelCollection::player_set(int player_index, QString name, QString value){
}


void WorkFilterModelCollection::master_trigger(QString name){
}

void WorkFilterModelCollection::master_set(QString name, bool value){
}

void WorkFilterModelCollection::master_set(QString name, int value){
}

void WorkFilterModelCollection::master_set(QString name, double value){
  if (name == "bpm") {
    if (mCurrentBPM == value)
      return;
    mCurrentBPM = value;
    mBPMTimeout->start(bpm_timer_timeout_ms);
  }
}

void WorkFilterModelCollection::select_work(int work_id) { emit(work_selected(work_id)); }
void WorkFilterModelCollection::update_history() { emit(history_updated()); }

void WorkFilterModelCollection::bpm_send_timeout() {
  if (mCurrentBPM != mLastBPM) {
    mLastBPM = mCurrentBPM;
    emit(current_bpm_changed(mCurrentBPM));
  }
}

