#include <QApplication>
#include "mixerpanel.hpp"
#include "player_view.hpp"
#include "audiomodel.hpp"
#include "playermapper.hpp"

using namespace DataJockey;

int main(int argc, char * argv[]){
   QApplication app(argc, argv);
   app.setStyle("plastique");

   Audio::AudioModel * model = Audio::AudioModel::instance();
   View::MixerPanel * mixer_panel = new View::MixerPanel();
   Controller::PlayerMapper * mapper = new Controller::PlayerMapper(&app);

   mapper->map(mixer_panel->players());

   model->start_audio();
   model->set_player_audio_file(0, "/home/alex/projects/music/11-phuture-acid_tracks.flac");
   model->set_player_audio_file(1, "/home/alex/projects/music/11-phuture-acid_tracks.flac");

   QObject::connect(qApp, SIGNAL(aboutToQuit()), model, SLOT(stop_audio()));

   mixer_panel->show();
   app.exec();
}
