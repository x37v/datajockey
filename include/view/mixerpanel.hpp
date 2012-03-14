#ifndef DATAJOCKEY_MIXERPANEL_VIEW_HPP
#define DATAJOCKEY_MIXERPANEL_VIEW_HPP

#include <QWidget>
#include <QList>
#include <QMap>
#include "beatbuffer.hpp"

class QSlider;
class QDoubleSpinBox;

namespace DataJockey {
   namespace View {
      class Player;

      class MixerPanel : public QWidget {
         Q_OBJECT
         public:
            MixerPanel(QWidget * parent = NULL);
            virtual ~MixerPanel();
            QList<Player *> players() const;
            QSlider * cross_fade_slider() const;;
            QSlider * master_volume_slider() const;;

         public slots:
            void set_player_toggle(int player_index, QString name, bool value);
            void set_player_int(int player_index, QString name, int value);

            void set_player_audio_file(int player_index, QString file_name);
            void set_player_beat_buffer(int player_index, Audio::BeatBuffer buffer);
            void set_player_song_description(int player_index, QString line1, QString line2);

            void set_tempo(double bpm);
            void set_master_int(QString name, int val);

         protected slots:
            void relay_player_toggle(bool state);
            void relay_player_seek_frame_relative(int frames);
            void relay_player_seeking(bool state);
            void relay_player_trigger();
            void relay_player_volume(int val);
            void relay_player_eq(int val);

         signals:
            void tempo_changed(double);
            void crossfade_changed(int val);

            void player_trigger(int player_index, QString name);
            void player_toggle(int player_index, QString name, bool val);
            void player_int(int player_index, QString name, int val);

         private:
            QList<Player *> mPlayers;
            QSlider * mCrossFadeSlider;
            QSlider * mMasterVolume;
            QDoubleSpinBox * mMasterTempo;
            bool mSettingTempo;
            
            //map senders to indices for relaying
            QMap<QObject *, int> mSenderToIndex;
      };
   }
}
#endif
