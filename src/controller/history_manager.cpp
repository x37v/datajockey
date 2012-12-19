#include "history_manager.hpp"
#include "db.hpp"
#include "audiomodel.hpp"
#include <QTimer>
#include <QSignalMapper>

#define LOG_DELAY_MS 5000

using namespace dj::model;
using namespace dj::controller;

HistoryManager::HistoryManager(QObject * parent) : QObject(parent) {
  QSignalMapper * mapper = new QSignalMapper(this);

  for (int i = 0; i < static_cast<int>(dj::audio::AudioModel::instance()->player_count()); i++) {
    mLoadedWorks.push_back(-1);
    QTimer * timer = new QTimer(this);
    timer->setSingleShot(true);
    mTimeouts.push_back(timer);
    QObject::connect(timer, SIGNAL(timeout()), mapper, SLOT(map()));
    mapper->setMapping(timer, i);
  }

  QObject::connect(mapper, SIGNAL(mapped(int)), SLOT(log_work(int)));
}

void HistoryManager::player_set(int player_index, QString name, int value) {
  if (name == "load") {
    mLoadedWorks[player_index] = value;
    mTimeouts[player_index]->stop();
  }
}

void HistoryManager::player_set(int player_index, QString name, bool value) {
  if (name == "audible") {
    if (value)
      mTimeouts[player_index]->start(LOG_DELAY_MS);
    else
      mTimeouts[player_index]->stop();
  }
}

void HistoryManager::log_work(int player_index) {
  int work = mLoadedWorks[player_index];
  if (work < 0)
    return;

  //only log the work once
  if (!mLoggedWorks.contains(work)) {
    db::work::set_played(work);
    mLoggedWorks[work] = true;
    emit(updated_history());
  }
}
