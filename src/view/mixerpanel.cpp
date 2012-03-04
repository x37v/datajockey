#include "mixerpanel.hpp"
#include "audiomodel.hpp"
#include "player_view.hpp"

using namespace DataJockey::View;

MixerPanel::MixerPanel(QWidget * parent) : QWidget(parent) {
   unsigned int num_players = Audio::AudioModel::instance()->player_count();

   QBoxLayout * layout = new QBoxLayout(QBoxLayout::LeftToRight, this);

   for (unsigned int i = 0; i < num_players; i++) {
      Player::WaveformOrientation orentation = Player::WAVEFORM_NONE;
      if (i == 0)
         orentation = Player::WAVEFORM_RIGHT;
      else if (i == 1)
         orentation = Player::WAVEFORM_LEFT;
      Player * player = new Player(this, orentation);
      mPlayers.append(player);
      layout->addWidget(player);
   }

   setLayout(layout);
}

MixerPanel::~MixerPanel() {
}

QList<Player *> MixerPanel::players() const { return mPlayers; }

