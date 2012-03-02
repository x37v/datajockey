#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>
#include <QTextEdit>

class QPushButton;
class QSlider;
class QDial;
class QProgressBar;

namespace DataJockey {
   namespace View {
      class Player : public QWidget {
         Q_OBJECT
         public:
            Player(QWidget * parent = NULL);
            QPushButton * button(QString name) const;
            QDial * eq_dial(QString name) const;
            QSlider * volume_slider() const;
            QProgressBar * progress_bar() const;
         private:
            QProgressBar * mProgressBar;
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QTextEdit * mTrackDescription;
            QSlider * mVolumeSlider;
            QMap<QString, QDial *> mEqDials;
      };
   }
}

#endif
