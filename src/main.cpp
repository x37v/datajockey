#include "audiobuffer.hpp"
#include "audioio.hpp"
#include "player.hpp"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char * argv[]){
	if(argc < 3)
		return -1;
	DataJockey::AudioIO audioio;
	DataJockey::AudioBuffer buffer(argv[1]);
	DataJockey::AudioBuffer buffer2(argv[2]);
	DataJockey::Player player;
	DataJockey::Player player2;

	audioio.master()->add_player(&player);
	audioio.master()->add_player(&player2);
	player.audio_buffer(&buffer);
	player.play_state(DataJockey::Player::PLAY);
	player.play_speed(0.95);

	player2.audio_buffer(&buffer2);
	player2.play_state(DataJockey::Player::PLAY);
	player2.play_speed(1.1);

	//player.out_state(DataJockey::Player::CUE);
	//player2.out_state(DataJockey::Player::MAIN_MIX);

	cout << "starting" << endl;
	audioio.start();
	audioio.connectToPhysical(0,0);
	audioio.connectToPhysical(1,1);

	//audioio.connectToPhysical(2,0);
	//audioio.connectToPhysical(3,1);

	audioio.master()->cross_fade(true);
	audioio.master()->cross_fade_mixers(0,1);
	audioio.master()->cross_fade_position(0);
	sleep(10);
	for(unsigned int i = 0; i <= 10; i++){
		audioio.master()->cross_fade_position((float)i * 0.1);
		cout << audioio.master()->cross_fade_position() << endl;
		sleep(1);
	}
	sleep(10);
	audioio.stop();
	sleep(1);

	return 0;
}

