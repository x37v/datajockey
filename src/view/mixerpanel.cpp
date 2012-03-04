#include "mixerpanel.hpp"
#include "audiomodel.hpp"
#include "player_view.hpp"
#include "defines.hpp"

#include <QSlider>

using namespace DataJockey::View;

MixerPanel::MixerPanel(QWidget * parent) : QWidget(parent) {
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

   top_layout->addLayout(mixer_layout);

   mCrossFadeSlider = new QSlider(Qt::Horizontal, this);
   mCrossFadeSlider->setRange(0, one_scale);
   top_layout->addWidget(mCrossFadeSlider);

   setLayout(top_layout);
}

MixerPanel::~MixerPanel() {
}

QList<Player *> MixerPanel::players() const { return mPlayers; }
QSlider * MixerPanel::cross_fade_slider() const { return mCrossFadeSlider; }
