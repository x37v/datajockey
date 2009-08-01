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
	DataJockey::Player player2;

	audioio.mixer()->add_player(&player);
	audioio.mixer()->add_player(&player2);
	player.mAudioBuffer = &buffer;
	player.play_state(DataJockey::Player::PLAY);
	player.play_speed(0.95);
	player.mute(true);

	player2.mAudioBuffer = &buffer;
	player2.play_state(DataJockey::Player::PLAY);
	player2.play_speed(1.1);

	player.out_state(DataJockey::Player::CUE);
	player2.out_state(DataJockey::Player::MAIN_MIX);

	cout << "starting" << endl;
	audioio.start();
	audioio.connectToPhysical(0,0);
	audioio.connectToPhysical(1,1);

	//audioio.connectToPhysical(2,0);
	//audioio.connectToPhysical(3,1);

	sleep(10);
	player2.mute(true);
	player.mute(false);

	sleep(60);
	audioio.stop();

	return 0;
}

