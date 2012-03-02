#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>
#include <QTextEdit>
#include <QSlider>

class QPushButton;

namespace DataJockey {
   namespace View {
      class Player : public QWidget {
         Q_OBJECT
         public:
            Player(QWidget * parent = NULL);
            QPushButton * button(QString name) const;
            QSlider * volume_slider() const;
         private:
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QTextEdit * mTrackDescription;
            QSlider * mVolumeSlider;
      };
   }
}

#endif
