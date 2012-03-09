#include "playermapper.hpp"
#include "player_view.hpp"
#include "timepoint.hpp"
#include "db.hpp"

#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QDial>
#include <QSqlRecord>

//XXX
#include <iostream>
using namespace std;

using namespace DataJockey::Controller;

namespace {
   const QString cFileQueryString(
         "select audio_files.location audio_file, annotation_files.location beat_file\n"
         "from audio_works\n"
         "\tjoin audio_files on audio_files.id = audio_works.audio_file_id\n"
         "\tjoin annotation_files on annotation_files.audio_work_id = audio_works.id\n"
         "where audio_works.id = ");

   const QString cWorkInfoQueryString(
         "select audio_works.name title,\n"
         "\tartists.name\n"
         "artist from audio_works"
         "\tinner join artist_audio_works on artist_audio_works.audio_work_id = audio_works.id\n"
         "\tinner join artists on artists.id = artist_audio_works.artist_id\n"
         "where audio_works.id = ");
}

PlayerMapper::PlayerMapper(QObject * parent) :
   QObject(parent), mCurrentwork(0),
   mFileQuery("", Model::db::get()),
   mWorkInfoQuery("", Model::db::get())
{ 
   mAudioModel = Audio::AudioModel::instance();
}

PlayerMapper::~PlayerMapper() { }

void PlayerMapper::map(QList<View::Player *> players) {
   for (int i = 0; i < players.size(); i++)
      map(i, players[i]);
}

void PlayerMapper::map(int index, View::Player * player) {
   if (mIndexPlayerMap.contains(index))
      return;

   mIndexPlayerMap[index] = player;
   mPlayerIndexMap[player] = index;
   mIndexPreSeekPauseState[index] = false;
   QObject::connect(mAudioModel,
         SIGNAL(player_audio_file_load_progress(int, int)),
         this,
         SLOT(file_load_progress(int, int)));
   QObject::connect(mAudioModel,
         SIGNAL(player_audio_file_changed(int, QString)),
         this,
         SLOT(file_changed(int, QString)));
   QObject::connect(mAudioModel,
         SIGNAL(player_beat_buffer_changed(int)),
         this,
         SLOT(beat_buffer_changed(int)));
   QObject::connect(mAudioModel,
         SIGNAL(player_position_changed(int, int)),
         this,
         SLOT(position_changed(int, int)));

   QObject::connect(player,
         SIGNAL(seek_relative(int)),
         this,
         SLOT(seek_relative(int)));
   QObject::connect(player,
         SIGNAL(seeking(bool)),
         this,
         SLOT(seeking(bool)));

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

   QObject::connect(mAudioModel,
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

void PlayerMapper::setWork(int id) {
   mCurrentwork = id;
}


void PlayerMapper::button_pressed() {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;
   int index = mButtonIndexMap[button];
   QString name = button->property("dj_name").toString();
   if (name == "seek_back") 
      mAudioModel->set_player_position_relative(index, Audio::TimePoint(-1,3));
   else if (name ==  "seek_forward") 
      mAudioModel->set_player_position_relative(index, Audio::TimePoint(0,1));
   else if (name == "reset") 
      mAudioModel->set_player_position(index, 0.0);
   else if (name == "load") {
      //build up query
      QString fileQueryStr(cFileQueryString);
      QString workQueryStr(cWorkInfoQueryString);
      QString id;
      id.setNum(mCurrentwork);
      fileQueryStr.append(id);
      workQueryStr.append(id);
      //execute
      mFileQuery.exec(fileQueryStr);
      QSqlRecord rec = mFileQuery.record();
      int audioFileCol = rec.indexOf("audio_file");
      int beatFileCol = rec.indexOf("beat_file");

		if(mFileQuery.first()){
			QString audiobufloc = mFileQuery.value(audioFileCol).toString();
			QString beatbufloc = mFileQuery.value(beatFileCol).toString();
         mAudioModel->set_player_buffers(index,
               audiobufloc,
               beatbufloc);

         mWorkInfoQuery.exec(workQueryStr);
			if(mWorkInfoQuery.first()){
				rec = mWorkInfoQuery.record();
				int titleCol = rec.indexOf("title");
				int artistCol = rec.indexOf("artist");
            mIndexPlayerMap[index]->set_song_description(
                  mWorkInfoQuery.value(artistCol).toString(),
                  mWorkInfoQuery.value(titleCol).toString());
         } else {
            mIndexPlayerMap[index]->set_song_description("unknown", "unknown");
         }
      }
   }

}

void PlayerMapper::button_toggled(bool state) {
   QPushButton * button = static_cast<QPushButton *>(QObject::sender());
   if (!button)
      return;
   int index = mButtonIndexMap[button];
   QString name = button->property("dj_name").toString();
   if (name == "cue") {
      mAudioModel->set_player_cue(index, state);
   } else if (name == "pause") {
      mAudioModel->set_player_pause(index, state);
   } else if (name == "sync") {
      mAudioModel->set_player_sync(index, state);
   }
}

void PlayerMapper::volume_changed(int value) {
   QSlider * slider = static_cast<QSlider *>(QObject::sender());
   if (!slider)
      return;
   int index = mSliderIndexMap[slider];
   mAudioModel->set_player_volume(index, value);
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
      mAudioModel->set_player_eq(index, 0, value);
   } else if (name == "dj_eq_mid") {
      mAudioModel->set_player_eq(index, 1, value);
   } else if (name == "dj_eq_high") {
      mAudioModel->set_player_eq(index, 2, value);
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

void PlayerMapper::beat_buffer_changed(int player_index) {
   View::Player * player = mIndexPlayerMap[player_index];
   if (!player)
      return;
   player->set_beat_buffer(mAudioModel->player_beat_buffer(player_index));
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
   mAudioModel->set_player_position_frame_relative(index, frames);
}

void PlayerMapper::seeking(bool start) {
   View::Player * player = static_cast<View::Player *>(QObject::sender());
   if (!player)
      return;
   int index = mPlayerIndexMap[player];
   if (start) {
      mIndexPreSeekPauseState[index] = mAudioModel->player_pause(index);
      mAudioModel->set_player_pause(index, true);
   } else {
      mAudioModel->set_player_pause(index, mIndexPreSeekPauseState[index]);
   }
}

