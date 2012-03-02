#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>
#include <QTextEdit>
#include <QSlider>
#include <QDial>

class QPushButton;

namespace DataJockey {
   namespace View {
      class Player : public QWidget {
         Q_OBJECT
         public:
            Player(QWidget * parent = NULL);
            QPushButton * button(QString name) const;
            QDial * eq_dial(QString name) const;
            QSlider * volume_slider() const;
         private:
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QTextEdit * mTrackDescription;
            QSlider * mVolumeSlider;
            QMap<QString, QDial *> mEqDials;
      };
   }
}

#endif
