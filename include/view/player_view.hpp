#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QMap>
#include <QString>
#include <QBoxLayout>
#include <QTextEdit>
#include <QGraphicsView>
#include <QList>
#include "waveformview.hpp"
#include "beatbuffer.hpp"

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
            QList<QPushButton *> buttons() const;
            QDial * eq_dial(QString name) const;
            QList<QDial *> eq_dials() const;
            QSlider * volume_slider() const;
            QProgressBar * progress_bar() const;
         public slots:
            void set_audio_file(const QString& file_name);
            void set_audio_frame(int frame);
            void set_beat_buffer(Audio::BeatBuffer buffer);
         signals:
            void seek_relative(int frames);
            //tells the controller that we are going to seek, may toggle pause
            void seeking(bool state);
         protected slots:
            void relay_seek_relative(int frames);
            void relay_mouse_button(bool down); //from waveform
         private:
            QProgressBar * mProgressBar;
            QMap<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QBoxLayout * mControlLayout;
            QTextEdit * mTrackDescription;
            QSlider * mVolumeSlider;
            QMap<QString, QDial *> mEqDials;
            WaveFormView * mWaveFormView;
            unsigned int mFrames;
            bool mWasPausedPreSeek;
      };
   }
}

#endif
