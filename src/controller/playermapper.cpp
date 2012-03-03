#include "playermapper.hpp"
#include "player_view.hpp"
#include <QProgressBar>

using namespace DataJockey::Controller;

PlayerMapper::PlayerMapper(QObject * parent) : QObject(parent) { 
   mAudioController = Audio::AudioController::instance();
}

PlayerMapper::~PlayerMapper() { }

void PlayerMapper::map(int index, View::Player * player) {
   mIndexPlayerMap[index] = player;
   mPlayerIndexMap[player] = index;
   QObject::connect(mAudioController,
         SIGNAL(player_audio_file_load_progress(int, int)),
         this,
         SLOT(file_load_progress(int, int)));
   QObject::connect(mAudioController,
         SIGNAL(player_audio_file_changed(int, QString)),
         this,
         SLOT(file_changed(int, QString)));
   QObject::connect(mAudioController,
         SIGNAL(player_position_changed(int, int)),
         this,
         SLOT(position_changed(int, int)));

}

void PlayerMapper::button_pressed() {
}

void PlayerMapper::button_toggled(bool state) {
}

void PlayerMapper::file_changed(int player_index, QString file_name) {
   View::Player * player = mIndexPlayerMap[player_index];
   if (!player)
      return;
   player->set_audio_file(file_name);
}

void PlayerMapper::file_cleared(int player_index) {
   View::Player * player = mIndexPlayerMap[player_index];
   if (!player)
      return;
}

void PlayerMapper::file_load_progress(int player_index, int progress) {
   View::Player * player = mIndexPlayerMap[player_index];
   if (!player)
      return;
   player->progress_bar()->setValue(progress);
}

void PlayerMapper::position_changed(int player_index, int frame) {
   View::Player * player = mIndexPlayerMap[player_index];
   if (!player)
      return;
   player->set_audio_frame(frame);
}
