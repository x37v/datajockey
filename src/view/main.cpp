#include <QApplication>
#include "player_view.hpp"
#include "audiomodel.hpp"
#include "playermapper.hpp"

using namespace DataJockey;

int main(int argc, char * argv[]){
   QApplication app(argc, argv);
   app.setStyle("plastique");

   Audio::AudioModel * model = Audio::AudioModel::instance();

   View::Player player;
   player.show();

   Controller::PlayerMapper * mapper = new Controller::PlayerMapper(&app);
   mapper->map(0, &player);

   model->start_audio();

   model->set_player_audio_file(0, "/home/alex/projects/music/11-phuture-acid_tracks.flac");
   app.exec();
}
