#include "audiobuffer.hpp"
#include "audioio.hpp"
#include "player.hpp"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char * argv[]){
	DataJockey::AudioIO audioio;
	DataJockey::AudioBuffer buffer(argv[1]);
	DataJockey::Player player;

	audioio.mixer()->add_player(&player);
	player.mAudioBuffer = &buffer;
	player.play_state(DataJockey::Player::PLAY);
	player.play_speed(0.95);

	cout << "starting" << endl;
	audioio.start();
	audioio.connectToPhysical(0,0);
	audioio.connectToPhysical(1,1);

	sleep(60);
	//sleep(10);
	//player.play_state(DataJockey::Player::PAUSE);
	//sleep(3);
	//player.play_state(DataJockey::Player::PLAY);
	//sleep(10);
	audioio.stop();

	return 0;
}

