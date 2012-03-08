#include "application.hpp"
#include "mixerpanel.hpp"
#include "player_view.hpp"
#include "audiomodel.hpp"
#include "playermapper.hpp"
#include "defines.hpp"
#include "beatbuffer.hpp"
#include "db.hpp"
#include "workdetailview.hpp"
#include "workdbview.hpp"
#include "worktablemodel.hpp"
#include "tagmodel.hpp"

#include <QSlider>
#include <QFile>
#include <QWidget>
#include <QSplitter>

using namespace DataJockey;

Application::Application(int & argc, char ** argv) : QApplication(argc, argv) {
   Model::db::setup("QMYSQL", "datajockey", "developer", "pass");
   mAudioModel = Audio::AudioModel::instance();
   mTop = new QWidget(0, Qt::Window);

   setStyle("plastique");

   QObject::connect(this, SIGNAL(aboutToQuit()), this, SLOT(pre_quit_actions()));

   //XXX need a parent for mixer_panel?
   View::MixerPanel * mixer_panel = new View::MixerPanel();
   Controller::PlayerMapper * mapper = new Controller::PlayerMapper(this);

   mapper->map(mixer_panel->players());
   QObject::connect(mixer_panel->cross_fade_slider(),
         SIGNAL(valueChanged(int)),
         mAudioModel,
         SLOT(set_master_cross_fade_position(int)));
   QObject::connect(mAudioModel,
         SIGNAL(master_cross_fade_position_changed(int)),
         mixer_panel->cross_fade_slider(),
         SLOT(setValue(int)));

   QObject::connect(mixer_panel->master_volume_slider(),
         SIGNAL(valueChanged(int)),
         mAudioModel,
         SLOT(set_master_volume(int)));
   QObject::connect(mAudioModel,
         SIGNAL(master_volume_changed(int)),
         mixer_panel->master_volume_slider(),
         SLOT(setValue(int)));

   QObject::connect(mixer_panel,
         SIGNAL(tempo_changed(double)),
         mAudioModel,
         SLOT(set_master_bpm(double)));
   QObject::connect(mAudioModel,
         SIGNAL(master_bpm_changed(double)),
         mixer_panel,
         SLOT(set_tempo(double)));

   mAudioModel->set_master_cross_fade_enable(true);
   mAudioModel->set_master_cross_fade_players(0, 1);
   mAudioModel->set_master_cross_fade_position(one_scale / 2);
   mAudioModel->set_master_bpm(120.0);

   mAudioModel->set_player_pause(0, true);
   mAudioModel->set_player_pause(1, true);

   mAudioModel->start_audio();

   mAudioModel->set_player_buffers(0,
         "/media/x/music/dj_slugo/dance_mania_ghetto_classics_vol_1/11-freaky_ride.flac",
         "/media/x/datajockey_annotation/recovered/3981-dj_slugo-dance_mania_ghetto_classics_vol_1-freaky_ride.yaml");
   mAudioModel->set_player_buffers(1,
         "/media/x/music/low_end_theory/3455/07-suck_it.flac",
         "/media/x/datajockey_annotation/recovered/3973-low_end_theory-3455-suck_it.yaml");

   TagModel * tag_model = new TagModel(Model::db::get(), mTop);
   WorkTableModel * work_table_model = new WorkTableModel(Model::db::get(), mTop);
   WorkDetailView * work_detail = new WorkDetailView(tag_model, Model::db::get(), mTop);
   WorkDBView * work_db_view = new WorkDBView(work_table_model, mTop);

   QFile file(":/style.qss");
   if(file.open(QFile::ReadOnly)){
      QString styleSheet = QLatin1String(file.readAll());
      this->setStyleSheet(styleSheet);
   }

   QBoxLayout * left_layout = new QBoxLayout(QBoxLayout::TopToBottom);
   QSplitter * splitter = new QSplitter(Qt::Vertical);
   splitter->addWidget(mixer_panel);
   splitter->addWidget(work_detail);
   left_layout->addWidget(splitter);

   QBoxLayout * top_layout = new QBoxLayout(QBoxLayout::LeftToRight);
   splitter = new QSplitter(Qt::Horizontal);
   QWidget * left = new QWidget;
   left->setLayout(left_layout);
   splitter->addWidget(left);
   splitter->addWidget(work_db_view);
   top_layout->addWidget(splitter);

   mTop->setLayout(top_layout);
   mTop->show();
}

void Application::pre_quit_actions() {
   mAudioModel->stop_audio();
   Model::db::close();
}
