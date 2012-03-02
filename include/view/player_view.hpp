#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>

class QPushButton;

namespace DataJockey {
   namespace View {
      class Player : public QWidget {
         Q_OBJECT
         public:
            Player(QWidget * parent = NULL);
         private:
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
      };
   }
}

#endif
