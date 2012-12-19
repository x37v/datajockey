#include "history_manager.hpp"
#include "db.hpp"

using namespace dj::model;
using namespace dj::controller;

HistoryManager::HistoryManager(QObject * parent) : QObject(parent) {
}

void HistoryManager::player_set(int player_index, QString name, int value) {
  if (name == "load")
    mLoadedWorks[player_index] = value;
}

void HistoryManager::player_set(int player_index, QString name, bool value) {
  if (name == "audible") {
    //XXX tmp
    if (!value)
      return;
    QHash<int, int>::iterator it = mLoadedWorks.find(player_index);
    if (it == mLoadedWorks.end())
      return;

    //only log the work once
    if (!mLoggedWorks.contains(*it)) {
      db::work::set_played(*it);
      mLoggedWorks[*it] = true;
      emit(updated_history());
    }
  }
}

