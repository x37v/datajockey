#ifndef DJ_HISTORY_MANAGER_HPP
#define DJ_HISTORY_MANAGER_HPP

#include <QObject>
#include <QHash>

namespace dj {
  namespace controller {
    class HistoryManager : public QObject {
      Q_OBJECT
      public:
        HistoryManager(QObject * parent = NULL);
      public slots:
        void player_set(int player_index, QString name, int value);
        void player_set(int player_index, QString name, bool value);
      private:
        QHash<int,int> mLoadedWorks;
        QHash<int,bool> mLoggedWorks;
    };
  }
}

#endif
