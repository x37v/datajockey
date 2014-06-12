#ifndef DJ_HISTORYMANAGER_H
#define DJ_HISTORYMANAGER_H

#include <QObject>
#include <QHash>
#include <QDateTime>

class QTimer;

class HistoryManager : public QObject {
  Q_OBJECT
  public:
    HistoryManager(int num_players, QObject * parent = NULL);
  public slots:
    void playerSetValueInt(int player_index, QString name, int value);
    void playerSetValueBool(int player_index, QString name, bool value);
  protected slots:
    void logWork(int player_index);
  signals:
    void workHistoryChanged(int work_id, QDateTime played_at);
  private:
    QList<int> mLoadedWorks;
    QList<QTimer *> mTimeouts;
    QHash<int,bool> mLoggedWorks;
};

#endif
