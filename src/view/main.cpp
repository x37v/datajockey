#include <QApplication>
#include "mixerpanel.hpp"
#include "player_view.hpp"
#include "audiomodel.hpp"
#include "playermapper.hpp"
#include "defines.hpp"
#include "beatbuffer.hpp"
#include "db.hpp"

#include <QSlider>
#include <QFile>

#include <iostream>

using std::cout;
using std::endl;

using namespace DataJockey;

int main(int argc, char * argv[]){
   QApplication app(argc, argv);
   app.setStyle("plastique");

   //Model::db::setup("QMYSQL", "datajockey", "developer", "pass");

   Audio::AudioModel * model = Audio::AudioModel::instance();
   View::MixerPanel * mixer_panel = new View::MixerPanel();
   Controller::PlayerMapper * mapper = new Controller::PlayerMapper(&app);

   mapper->map(mixer_panel->players());
   QObject::connect(mixer_panel->cross_fade_slider(),
         SIGNAL(valueChanged(int)),
         model,
         SLOT(set_master_cross_fade_position(int)));
   QObject::connect(model,
         SIGNAL(master_cross_fade_position_changed(int)),
         mixer_panel->cross_fade_slider(),
         SLOT(setValue(int)));

   QObject::connect(mixer_panel->master_volume_slider(),
         SIGNAL(valueChanged(int)),
         model,
         SLOT(set_master_volume(int)));
   QObject::connect(model,
         SIGNAL(master_volume_changed(int)),
         mixer_panel->master_volume_slider(),
         SLOT(setValue(int)));

   QObject::connect(mixer_panel,
         SIGNAL(tempo_changed(double)),
         model,
         SLOT(set_master_bpm(double)));
   QObject::connect(model,
         SIGNAL(master_bpm_changed(double)),
         mixer_panel,
         SLOT(set_tempo(double)));

   model->set_master_cross_fade_enable(true);
   model->set_master_cross_fade_players(0, 1);
   model->set_master_cross_fade_position(one_scale / 2);
   model->set_master_bpm(120.0);

   model->set_player_pause(0, true);
   model->set_player_pause(1, true);

   model->start_audio();

   model->set_player_buffers(0,
         "/media/x/music/dj_slugo/dance_mania_ghetto_classics_vol_1/11-freaky_ride.flac",
         "/media/x/datajockey_annotation/recovered/3981-dj_slugo-dance_mania_ghetto_classics_vol_1-freaky_ride.yaml");
   model->set_player_buffers(1,
         "/media/x/music/low_end_theory/3455/07-suck_it.flac",
         "/media/x/datajockey_annotation/recovered/3973-low_end_theory-3455-suck_it.yaml");

   //model->set_player_buffers(0,
         //"/media/x/music/various/warp102-influences/disc_1/11-phuture-acid_tracks.flac",
         //"/media/x/datajockey_annotation/3029.yaml");
   //model->set_player_buffers(1,
         //"/media/x/music/various/warp102-influences/disc_1/11-phuture-acid_tracks.flac",
         //"/media/x/datajockey_annotation/3029.yaml");

   QObject::connect(qApp, SIGNAL(aboutToQuit()), model, SLOT(stop_audio()));

   QFile file(":/style.qss");
   if(file.open(QFile::ReadOnly)){
      QString styleSheet = QLatin1String(file.readAll());
      app.setStyleSheet(styleSheet);
   }

   mixer_panel->show();
   app.exec();
}
