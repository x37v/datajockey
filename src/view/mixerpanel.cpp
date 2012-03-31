#include "mixerpanel.hpp"
#include "audiomodel.hpp"
#include "player_view.hpp"
#include "audiolevel.hpp"
#include "defines.hpp"

#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QDial>
#include <QProgressBar>

#include <iostream>
using std::cout;
using std::endl;

using namespace DataJockey::View;

MixerPanel::MixerPanel(QWidget * parent) : QWidget(parent), mSettingTempo(false) {
   unsigned int num_players = Audio::AudioModel::instance()->player_count();

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
      
      QObject::connect(player->volume_slider(),
            SIGNAL(valueChanged(int)),
            this,
            SLOT(relay_player_volume(int)));

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
                  SLOT(relay_player_toggled(bool)));
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

   //master
   QBoxLayout * master_layout = new QBoxLayout(QBoxLayout::TopToBottom);
   master_layout->setAlignment(Qt::AlignCenter);

   QLabel * lab = new QLabel("Tempo", this);
   master_layout->addWidget(lab, 0, Qt::AlignHCenter);

   mMasterTempo = new QDoubleSpinBox(this);
   mMasterTempo->setValue(120.0);
   mMasterTempo->setRange(10.0, 240.0);
   mMasterTempo->setSingleStep(0.5);
   mMasterTempo->setDecimals(3);
   master_layout->addWidget(mMasterTempo, 0, Qt::AlignHCenter);

   mMasterVolume = new QSlider(Qt::Vertical, this);
   mMasterVolume->setRange(0, 1.5 * one_scale);
   mMasterVolume->setValue(one_scale);

   mAudioLevel = new AudioLevel(this);

   QBoxLayout * slider_level_layout = new QBoxLayout(QBoxLayout::LeftToRight);
   slider_level_layout->addStretch(10);
   slider_level_layout->addWidget(mMasterVolume, 1, Qt::AlignHCenter);
   slider_level_layout->addWidget(mAudioLevel, 1, Qt::AlignHCenter);
   slider_level_layout->addStretch(10);
   slider_level_layout->setContentsMargins(0,0,0,0);

   master_layout->addLayout(slider_level_layout);

   mixer_layout->addLayout(master_layout);
   top_layout->addLayout(mixer_layout);

   mCrossFadeSlider = new QSlider(Qt::Horizontal, this);
   mCrossFadeSlider->setRange(0, one_scale);
   top_layout->addWidget(mCrossFadeSlider);

   setLayout(top_layout);

   QObject::connect(mMasterTempo, SIGNAL(valueChanged(double)), this, SLOT(relay_tempo_changed(double)));
   QObject::connect(mCrossFadeSlider, SIGNAL(valueChanged(int)), this, SLOT(relay_crossfade_changed(int)));
   QObject::connect(mMasterVolume, SIGNAL(valueChanged(int)), this, SLOT(relay_volume_changed(int)));
}

MixerPanel::~MixerPanel() {
}

void MixerPanel::player_set(int player_index, QString name, bool value) {
   if (player_index >= mPlayers.size())
      return;
   if(QPushButton * button = mPlayers[player_index]->button(name))
      button->setChecked(value);
}

void MixerPanel::player_set(int player_index, QString name, int value) {
   if (player_index >= mPlayers.size())
      return;

   Player * player = mPlayers[player_index];
   if (name == "volume")
      player->volume_slider()->setValue(value);
   else if (name == "frame")
      player->set_audio_frame(value);
   else if (name == "progress")
      player->progress_bar()->setValue(value);
   else if (name == "eq_low")
      player->eq_dial("low")->setValue(value);
   else if (name == "eq_mid")
      player->eq_dial("mid")->setValue(value);
   else if (name == "eq_high")
      player->eq_dial("high")->setValue(value);
   else if (name == "audio_level")
      player->set_audio_level(value);
}

void MixerPanel::player_set(int player_index, QString name, QString value) {
   if (player_index >= mPlayers.size())
      return;

   Player * player = mPlayers[player_index];
   if (name == "audio_file")
      player->set_audio_file(value);
   else if (name == "song_description")
      player->set_song_description(value);
}

void MixerPanel::player_set_beat_buffer(int player_index, Audio::BeatBuffer buffer){
   if (player_index >= mPlayers.size())
      return;
   mPlayers[player_index]->set_beat_buffer(buffer);
}

void MixerPanel::master_set(QString name, int val) {
   if (name == "volume")
      mMasterVolume->setValue(val);
   else if (name == "crossfade_position")
      mCrossFadeSlider->setValue(val);
   else if (name == "audio_level")
      mAudioLevel->set_level(val);
}

void MixerPanel::master_set(QString name, double val) {
   if (name == "bpm")
      mMasterTempo->setValue(val);
}

void MixerPanel::relay_player_toggled(bool checked) {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;

   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(button);
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_toggled(player_index.value(), button->property("dj_name").toString(), checked));
}

void MixerPanel::relay_player_triggered() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;

   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(button);
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_triggered(player_index.value(), button->property("dj_name").toString()));
}

void MixerPanel::relay_player_volume(int val) {
   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "volume", val));
}

void MixerPanel::relay_player_seek_frame_relative(int frames) {
   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_value_changed(player_index.value(), "play_frame_relative", frames));
}

void MixerPanel::relay_player_seeking(bool state) {
   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
   if (player_index == mSenderToIndex.end())
      return;

   emit(player_toggled(player_index.value(), "seeking", state));
}

void MixerPanel::relay_player_eq(int val) {
   QMap<QObject *, int>::const_iterator player_index = mSenderToIndex.find(sender());
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
   emit(master_value_changed("bpm", value));
}

