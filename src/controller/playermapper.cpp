#include "playermapper.hpp"
#include "player_view.hpp"
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QDial>

//XXX
#include <iostream>
using namespace std;

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

   QObject::connect(player,
         SIGNAL(seek_relative(int)),
         this,
         SLOT(seek_relative(int)));

   QPushButton * button;
   foreach(button, player->buttons()) {
      mIndexButtonMap[index] = button;
      mButtonIndexMap[button] = index;

      if (button->isCheckable()) {
         QObject::connect(button,
               SIGNAL(toggled(bool)),
               this,
               SLOT(button_toggled(bool)));
      } else {
         QObject::connect(button,
               SIGNAL(pressed()),
               this,
               SLOT(button_pressed()));
      }
   }

   mSliderIndexMap[player->volume_slider()] =  index;
   mIndexSliderMap[index] = player->volume_slider();

   QObject::connect(mAudioController,
         SIGNAL(player_volume_changed(int, int)),
         this,
         SLOT(volume_changed(int, int)));
   QObject::connect(player->volume_slider(),
         SIGNAL(valueChanged(int)),
         this,
         SLOT(volume_changed(int)));

   mSliderIndexMap[player->volume_slider()] =  index;
   mIndexSliderMap[index] = player->volume_slider();

   QDial * dial;
   foreach(dial, player->eq_dials()) {
      mIndexDialMap[index] = dial;
      mDialIndexMap[dial] = index;
      QObject::connect(dial,
            SIGNAL(valueChanged(int)),
            this,
            SLOT(eq_changed(int)));
   }
}

void PlayerMapper::button_pressed() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;
   int index = mButtonIndexMap[button];
   QString name = button->property("dj_name").toString();
   if (name == "seek_back") 
      mAudioController->set_player_position_relative(index, -0.5);
   else if (name ==  "seek_forward") 
      mAudioController->set_player_position_relative(index, 0.5);
   else if (name == "reset") 
      mAudioController->set_player_position(index, 0.0);
   //else if (name == "load") 

}

void PlayerMapper::button_toggled(bool state) {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;
   int index = mButtonIndexMap[button];
   QString name = button->property("dj_name").toString();
   if (name == "cue") {
      mAudioController->set_player_cue(index, state);
   } else if (name == "pause") {
      mAudioController->set_player_pause(index, state);
   } else if (name == "sync") {
      mAudioController->set_player_sync(index, state);
   }
}

void PlayerMapper::volume_changed(int value) {
   QSlider * slider = static_cast<QSlider *>(QObject::sender());
   if (!slider)
      return;
   int index = mSliderIndexMap[slider];
   mAudioController->set_player_volume(index, value);
}

void PlayerMapper::volume_changed(int player_index, int value) {
   QSlider * slider = mIndexSliderMap[player_index];
   if (slider)
      slider->setValue(value);
}

void PlayerMapper::eq_changed(int value) {
   QDial * eq = static_cast<QDial *>(QObject::sender());
   if (!eq)
      return;
   int index = mDialIndexMap[eq];

   QString name = eq->property("dj_name").toString();
   if (name == "dj_eq_low") {
      mAudioController->set_player_eq(index, 0, value);
   } else if (name == "dj_eq_mid") {
      mAudioController->set_player_eq(index, 1, value);
   } else if (name == "dj_eq_high") {
      mAudioController->set_player_eq(index, 2, value);
   }
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

void PlayerMapper::seek_relative(int frames) {
   View::Player * player = static_cast<View::Player *>(QObject::sender());
   if (!player)
      return;
   int index = mPlayerIndexMap[player];
   mAudioController->set_player_position_frame_relative(index, frames);
}

