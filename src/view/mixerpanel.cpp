#include "mixerpanel.hpp"
#include "audiomodel.hpp"
#include "playerview.hpp"
#include "audiolevel.hpp"
#include "defines.hpp"

#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDial>
#include <QProgressBar>
#include <QLineEdit>
#include <QSignalMapper>

#include <iostream>
using std::cout;
using std::endl;

using namespace dj::view;

MixerPanel::MixerPanel(QWidget * parent) : QWidget(parent), mSettingTempo(false), mMasterPositionLast(0,0,0) {
   unsigned int num_players = audio::AudioModel::instance()->player_count();

   QBoxLayout * top_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
   QBoxLayout * mixer_layout = new QBoxLayout(QBoxLayout::LeftToRight);

   for (unsigned int i = 0; i < num_players; i++) {
      Player::WaveformOrientation orentation = Player::WAVEFORM_NONE;
      if (i == 0)
         orentation = Player::WAVEFORM_RIGHT;
      else if (i == 1)
         orentation = Player::WAVEFORM_LEFT;

      Player * player = new Player(this, orentation);
      mPlayers.append(player);
      mixer_layout->addWidget(player);

      //hacks, setting values to map objects to player indices
      mSenderToIndex[player] = i;
      mSenderToIndex[player->volume_slider()] = i;
      mSenderToIndex[player->speed_view()] = i;
      
      QObject::connect(player->volume_slider(),
            SIGNAL(valueChanged(int)),
            this,
            SLOT(relay_player_volume(int)));

      QObject::connect(player->speed_view(),
            SIGNAL(valueChanged(int)),
            this,
            SLOT(relay_player_speed(int)));

      QObject::connect(player,
            SIGNAL(seeking(bool)),
            this,
            SLOT(relay_player_seeking(bool)));
      QObject::connect(player,
            SIGNAL(seek_frame_relative(int)),
            this,
            SLOT(relay_player_seek_frame_relative(int)));

      QPushButton * button;
      foreach(button, player->buttons()) {
         mSenderToIndex[button] = i;
         if (button->isCheckable()) {
            QObject::connect(button,
                  SIGNAL(toggled(bool)),
                  this,
                  SLOT(relay_player_value_changed(bool)));
         } else {
            QObject::connect(button,
                  SIGNAL(clicked()),
                  this,
                  SLOT(relay_player_triggered()));
         }
      }
      QDial * dial;
      foreach(dial, player->eq_dials()) {
         mSenderToIndex[dial] = i;
         QObject::connect(dial,
               SIGNAL(valueChanged(int)),
               this,
               SLOT(relay_player_eq(int)));
      }
   }

   //set up the midi map button
   QPushButton * midi_map = new QPushButton("midi learn", this);
   midi_map->setCheckable(false);
   QObject::connect(midi_map,
         SIGNAL(pressed()),
         SIGNAL(midi_map_triggered()));

   //master
   QBoxLayout * master_layout = new QBoxLayout(QBoxLayout::TopToBottom);
   master_layout->setAlignment(Qt::AlignCenter);

   //position
   mMasterPosition = new QLineEdit("000:0", this);
   master_layout->addWidget(mMasterPosition, 0, Qt::AlignHCenter);
   mMasterPosition->setReadOnly(true);
   mMasterPosition->setAlignment(Qt::AlignRight);

   QLabel * lab = new QLabel("Tempo", this);
   master_layout->addWidget(lab, 0, Qt::AlignHCenter);

   mMasterTempo = new QDoubleSpinBox(this);
   mMasterTempo->setValue(120.0);
   mMasterTempo->setRange(10.0, 240.0);
   mMasterTempo->setSingleStep(0.5);
   mMasterTempo->setDecimals(3);
   master_layout->addWidget(mMasterTempo, 0, Qt::AlignHCenter);

   master_layout->addWidget(midi_map, 0, Qt::AlignHCenter);

   //set up sync buttons
   QSignalMapper * sync_mapper = new QSignalMapper(this);
   for (int i = 0; i < (int)num_players; i++) {
      QPushButton * sync_button = new QPushButton("sync to " + QString::number(i + 1), this);
      mPlayerCanSync <<  true;
      mSyncToPlayer << sync_button;
      sync_button->setCheckable(false);
      sync_button->setDisabled(true);

      master_layout->addWidget(sync_button);
      QObject::connect(
            sync_button, SIGNAL(clicked()),
            sync_mapper, SLOT(map()));
      sync_mapper->setMapping(sync_button, i);
   }
   QObject::connect(
         sync_mapper, SIGNAL(mapped(int)),
         SLOT(relay_master_sync(int)));

   mMasterVolume = new QSlider(Qt::Vertical, this);
   mMasterVolume->setRange(0, static_cast<int>(1.5 * static_cast<float>(one_scale)));
   mMasterVolume->setValue(one_scale);
   mMasterVolume->setMinimumHeight(dj::volume_slider_height);
   mMasterVolume->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

   mAudioLevel = new AudioLevel(this);

   mSliderLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight);
   mSliderLevelLayout->addStretch(10);
   mSliderLevelLayout->addWidget(mMasterVolume, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addWidget(mAudioLevel, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addStretch(10);
   mSliderLevelLayout->setContentsMargins(0,0,0,0);

   master_layout->addStretch(10);
   master_layout->addLayout(mSliderLevelLayout);

   mixer_layout->addLayout(master_layout);
   mixer_layout->setContentsMargins(0, 0, 0, 0);
   top_layout->addLayout(mixer_layout);

   mCrossFadeSlider = new QSlider(Qt::Horizontal, this);
   mCrossFadeSlider->setRange(0, one_scale);
   top_layout->addWidget(mCrossFadeSlider);

   setLayout(top_layout);

   top_layout->setSizeConstraint(QLayout::SetMinimumSize);

   QObject::connect(mMasterTempo, SIGNAL(valueChanged(double)), this, SLOT(relay_tempo_changed(double)));
   QObject::connect(mCrossFadeSlider, SIGNAL(valueChanged(int)), this, SLOT(relay_crossfade_changed(int)));
   QObject::connect(mMasterVolume, SIGNAL(valueChanged(int)), this, SLOT(relay_volume_changed(int)));
}

MixerPanel::~MixerPanel() {
}

void MixerPanel::player_set(int player_index, QString name, bool value) {
   if (player_index >= mPlayers.size())
      return;
   if (name == "update_sync_disabled") {
      mPlayerCanSync[player_index] = !value;
      mPlayers[player_index]->button("sync")->setDisabled(value);
      //disable sync to player if syncing isn't available or player is in sync running mode
      mSyncToPlayer[player_index]->setDisabled(value || mPlayers[player_index]->button("sync")->isChecked());
   } else if(QPushButton * button = mPlayers[player_index]->button(name)) {
      button->setChecked(value);
      //only enable sync to player when the player is running in free mode and it can sync
      if (name == "sync") {
         if (!value && mPlayerCanSync[player_index])
            mSyncToPlayer[player_index]->setDisabled(false);
         else
            mSyncToPlayer[player_index]->setDisabled(true);
      }
   }
}

void MixerPanel::player_set(int player_index, QString name, int value) {
   if (player_index >= mPlayers.size())
      return;

   Player * player = mPlayers[player_index];
   if (name == "volume")
      player->volume_slider()->setValue(value);
   else if (name == "speed" || name == "update_speed")
      player->speed_view()->setValue(value);
   else if (name == "update_frame")
      player->set_audio_frame(value);
   else if (name == "update_progress")
      player->progress_bar()->setValue(value);
   else if (name == "eq_low")
      player->eq_dial("low")->setValue(value);
   else if (name == "eq_mid")
      player->eq_dial("mid")->setValue(value);
   else if (name == "eq_high")
      player->eq_dial("high")->setValue(value);
   else if (name == "update_audio_level")
      player->set_audio_level(value);
}

void MixerPanel::player_set(int player_index, QString name, QString value) {
   if (player_index >= mPlayers.size())
      return;

   Player * player = mPlayers[player_index];
   if (name == "song_description")
      player->set_song_description(value);
}

void MixerPanel::player_set_buffers(int player_index, audio::AudioBufferPtr audio_buffer, audio::BeatBufferPtr beat_buffer) {
   if (player_index >= mPlayers.size())
      return;
   mPlayers[player_index]->set_buffers(audio_buffer, beat_buffer);
}

void MixerPanel::master_set(QString name, int val) {
   if (name == "volume")
      mMasterVolume->setValue(val);
   else if (name == "crossfade_position")
      mCrossFadeSlider->setValue(val);
   else if (name == "update_audio_level")
      mAudioLevel->set_level(val);
}

void MixerPanel::master_set(QString name, double val) {
   if (name == "bpm") {
      mSettingTempo = true;
      mMasterTempo->setValue(val);
      mSettingTempo = false;
   }
}

void MixerPanel::master_set(QString name, audio::TimePoint val) {
   if (name == "update_transport_position") {
      //ignore position within beat
      val.pos_in_beat(0);
      if (val != mMasterPositionLast) {
         QString bar;
         QString beat;
         bar.setNum(val.bar());
         beat.setNum(val.beat() + 1);
         //mMasterPosition->setCursorPosition(0);
         mMasterPosition->setText(bar + ":" + beat);
         mMasterPositionLast = val;
      }
   }
}

void MixerPanel::relay_player_value_changed(bool checked) {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;

   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(button);
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), button->property("dj_name").toString(), checked));
}

void MixerPanel::relay_player_triggered() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;

   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(button);
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_triggered(player_index.value(), button->property("dj_name").toString()));
}

void MixerPanel::relay_player_volume(int val) {
   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "volume", val));
}

void MixerPanel::relay_player_speed(int val) {
   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "speed", val));
}

void MixerPanel::relay_player_seek_frame_relative(int frames) {
   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "play_frame_relative", frames));
}

void MixerPanel::relay_player_seeking(bool state) {
   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "seeking", state));
}

void MixerPanel::relay_player_eq(int val) {
   QHash<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), sender()->property("dj_name").toString(), val));
}

void MixerPanel::relay_crossfade_changed(int value) {
   emit(master_value_changed("crossfade_position", value));
}

void MixerPanel::relay_volume_changed(int value) {
   emit(master_value_changed("volume", value));
}

void MixerPanel::relay_tempo_changed(double value) {
   if (!mSettingTempo)
      emit(master_value_changed("bpm", value));
}

void MixerPanel::relay_master_sync(int player) {
   emit(master_value_changed("sync_to_player", player));
}

