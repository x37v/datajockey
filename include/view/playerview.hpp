#ifndef PLAYER_VIEW_HPP
#define PLAYER_VIEW_HPP

#include <QWidget>
#include <QHash>
#include <QString>
#include <QBoxLayout>
#include <QLineEdit>
#include <QGraphicsView>
#include <QList>
#include <QSpinBox>
#include <QValidator>
#include <QDoubleValidator>
#include "beatbuffer.hpp"
#include "audiobuffer.hpp"

class QPushButton;
class QSlider;
class QDial;
class QProgressBar;
class QGraphicsScene;
class QTimer;

namespace dj {
   namespace view {
      class SpeedSpinBox : public QSpinBox {
         Q_OBJECT
         public:
            SpeedSpinBox(QWidget * parent = NULL);
         protected:
            virtual QString textFromValue(int value) const;
            virtual int valueFromText(const QString & text) const;
            virtual QValidator::State validate(QString & text, int & pos) const;
         private:
            QDoubleValidator * mValidator;
      };

      class WaveFormViewGL;
      class AudioLevel;
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
            QSpinBox * speed_view() const;
            QProgressBar * progress_bar() const;
            QRect slider_level_geometry() const;
         public slots:
            void set_audio_level(int percent);
            void set_buffers(audio::AudioBufferPtr audio_buffer, audio::BeatBufferPtr beat_buffer);
            void set_audio_frame(int frame);
            void set_song_description(QString description);
            void set_cuepoint(int id, int frame);
         signals:
            void seek_to_frame(int frame);
            void seek_frame_relative(int frames);
            //tells the controller that we are going to seek, may toggle pause
            void seeking(bool state);
         private:
            QProgressBar * mProgressBar;
            QHash<QString, QPushButton *> mButtons;
            QBoxLayout * mTopLayout;
            QBoxLayout * mControlLayout;
            QLineEdit * mTrackDescription[2];
            QSlider * mVolumeSlider;
            QSpinBox * mSpeedView;
            QHash<QString, QDial *> mEqDials;
            WaveFormViewGL * mWaveFormZoomedView;
            WaveFormViewGL * mWaveFormFullView;
            AudioLevel * mAudioLevelView;
            QBoxLayout * mSliderLevelLayout;
            unsigned int mFrames;
            bool mWasPausedPreSeek;
      };
   }
}

#endif
