#ifndef DJ_HISTORY_MANAGER_HPP
#define DJ_HISTORY_MANAGER_HPP

#include <QObject>
#include <QHash>
#include <QDateTime>

class QTimer;

namespace dj {
  namespace controller {
    class HistoryManager : public QObject {
      Q_OBJECT
      public:
        HistoryManager(QObject * parent = NULL);
      public slots:
        void player_set(int player_index, QString name, int value);
        void player_set(int player_index, QString name, bool value);
      protected slots:
        void log_work(int player_index);
      signals:
        void updated_history(int work_id, QDateTime played_at);
      private:
        QList<int> mLoadedWorks;
        QList<QTimer *> mTimeouts;
        QHash<int,bool> mLoggedWorks;
    };
  }
}

#endif
