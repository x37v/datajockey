#include "workfiltermodelcollection.hpp"
#include "workfiltermodel.hpp"
#include "db.hpp"

using namespace dj;

WorkFilterModelCollection::WorkFilterModelCollection(QObject * parent, QSqlDatabase db) :
  QObject(parent),
  mCurrentBPM(120.0)
{
}

WorkFilterModel * WorkFilterModelCollection::new_filter_model() {
  WorkFilterModel * m = new WorkFilterModel(this, model::db::get());
  mFilterModels << m;
  m->set_current_bpm(mCurrentBPM);

  QObject::connect(this, SIGNAL(current_bpm_changed(double)), m, SLOT(set_current_bpm(double)));
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
    emit(current_bpm_changed(mCurrentBPM));
  }
}

void WorkFilterModelCollection::select_work(int work_id) { emit(work_selected(work_id)); }

