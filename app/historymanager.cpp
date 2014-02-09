#include "historymanager.h"
#include <QTimer>

#define LOG_DELAY_S 5

HistoryManager::HistoryManager(int num_players, QObject * parent) : QObject(parent) {
  for (int i = 0; i < num_players; i++) {
    mLoadedWorks.push_back(-1);
    QTimer * timer = new QTimer(this);
    timer->setSingleShot(true);
    mTimeouts.push_back(timer);
    connect(timer, &QTimer::timeout, [this, i]() { logWork(i); });
  }
}

void HistoryManager::playerSetValueInt(int player_index, QString name, int value) {
  if (name == "loading_work") {
    mLoadedWorks[player_index] = value;
    mTimeouts[player_index]->stop();
  }
}

void HistoryManager::playerSetValueBool(int player_index, QString name, bool value) {
  if (name == "audible") {
    if (value)
      mTimeouts[player_index]->start(LOG_DELAY_S * 1000);
    else
      mTimeouts[player_index]->stop();
  }
}

void HistoryManager::logWork(int player_index) {
  int work = mLoadedWorks[player_index];
  if (work < 0)
    return;

  //XXX only log the work once
  if (!mLoggedWorks.contains(work)) {
    QDateTime time = QDateTime::currentDateTime().addSecs(-LOG_DELAY_S);
    mLoggedWorks[work] = true;
    emit(workHistoryChanged(work, time));
  }
}

