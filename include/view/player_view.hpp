#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>
#include <QTextEdit>
#include <QGraphicsView>
#include "waveformview.hpp"

class QPushButton;
class QSlider;
class QDial;
class QProgressBar;
class QGraphicsScene;

namespace DataJockey {
   namespace View {
      class WaveFormItem;

      class Player : public QWidget {
         Q_OBJECT
         public:
            enum WaveformOrientation { WAVEFORM_NONE, WAVEFORM_LEFT, WAVEFORM_RIGHT };
            Player(QWidget * parent = NULL, WaveformOrientation waveform_orientation = WAVEFORM_RIGHT);
            QPushButton * button(QString name) const;
            QDial * eq_dial(QString name) const;
            QSlider * volume_slider() const;
            QProgressBar * progress_bar() const;
         public slots:
            void set_audio_file(const QString& file_name);
            void set_audio_frame(int frame);
         private:
            QProgressBar * mProgressBar;
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QBoxLayout * mControlLayout;
            QTextEdit * mTrackDescription;
            QSlider * mVolumeSlider;
            QMap<QString, QDial *> mEqDials;
            WaveFormView * mWaveFormView;
      };
   }
}

#endif
