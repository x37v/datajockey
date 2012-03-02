#include <QApplication>
#include "player_view.hpp"

using namespace DataJockey;

int main(int argc, char * argv[]){
   QApplication app(argc, argv);

   View::Player player;
   player.show();

   app.exec();
}
