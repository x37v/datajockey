#ifndef DATAJOCKEY_MIXERPANEL_VIEW_HPP
#define DATAJOCKEY_MIXERPANEL_VIEW_HPP

#include <QWidget>
#include <QList>
#include <QMap>
#include "beatbuffer.hpp"
#include "timepoint.hpp"

class QSlider;
class QDoubleSpinBox;
class QBoxLayout;

namespace DataJockey {
   namespace View {
      using DataJockey::Audio::TimePoint;

      class Player;
      class AudioLevel;

      class MixerPanel : public QWidget {
         Q_OBJECT
         public:
            MixerPanel(QWidget * parent = NULL);
            virtual ~MixerPanel();
         public slots:
            void player_set(int player_index, QString name, bool value);
            void player_set(int player_index, QString name, int value);
            void player_set(int player_index, QString name, QString value);
            void player_set_beat_buffer(int player_index, Audio::BeatBuffer buffer);

            void master_set(QString name, int val);
            void master_set(QString name, double val);
            void master_set(QString name, TimePoint val);

         protected slots:
            void relay_player_toggled(bool state);
            void relay_player_seek_frame_relative(int frames);
            void relay_player_seeking(bool state);
            void relay_player_triggered();
            void relay_player_volume(int val);
            void relay_player_eq(int val);

            void relay_crossfade_changed(int value);
            void relay_volume_changed(int value);
            void relay_tempo_changed(double value);

         protected:
            virtual void resizeEvent(QResizeEvent * event);

         signals:
            void player_triggered(int player_index, QString name);
            void player_toggled(int player_index, QString name, bool val);
            void player_value_changed(int player_index, QString name, int val);

            void tempo_changed(double);
            void master_value_changed(QString name, int value);
            void master_value_changed(QString name, double value);

         private:
            QList<Player *> mPlayers;
            QSlider * mCrossFadeSlider;
            QSlider * mMasterVolume;
            QDoubleSpinBox * mMasterTempo;
            bool mSettingTempo;
            AudioLevel * mAudioLevel;
            QBoxLayout * mSliderLevelLayout;
            
            //map senders to indices for relaying
            QMap<QObject *, int> mSenderToIndex;
      };
   }
}
#endif
