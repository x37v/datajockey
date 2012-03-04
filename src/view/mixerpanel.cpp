#include "mixerpanel.hpp"
#include "audiomodel.hpp"
#include "player_view.hpp"
#include "defines.hpp"

#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>

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
   master_layout->addWidget(mMasterVolume, 10, Qt::AlignHCenter);

   mixer_layout->addLayout(master_layout);
   top_layout->addLayout(mixer_layout);

   mCrossFadeSlider = new QSlider(Qt::Horizontal, this);
   mCrossFadeSlider->setRange(0, one_scale);
   top_layout->addWidget(mCrossFadeSlider);

   setLayout(top_layout);

   QObject::connect(mMasterTempo, SIGNAL(valueChanged(double)), this, SIGNAL(tempo_changed(double)));
}

MixerPanel::~MixerPanel() {
}

QList<Player *> MixerPanel::players() const { return mPlayers; }
QSlider * MixerPanel::cross_fade_slider() const { return mCrossFadeSlider; }
QSlider * MixerPanel::master_volume_slider() const { return mMasterVolume; }

void MixerPanel::set_tempo(double bpm) {
   if (mSettingTempo)
      return;

   mSettingTempo = true;
   mMasterTempo->setValue(bpm);
   emit(tempo_changed(bpm));
   mSettingTempo = false;
}

