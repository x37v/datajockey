#include "audiobuffer.hpp"
#include "audioio.hpp"
#include "player.hpp"
#include "master.hpp"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char * argv[]){
	if(argc < 3)
		return -1;
	DataJockey::AudioIO audioio;
	DataJockey::AudioBuffer buffer(argv[1]);
	DataJockey::AudioBuffer buffer2(argv[2]);
	DataJockey::Master * master = audioio.master();
	master->add_player();
	master->add_player();

	master->players()[0]->audio_buffer(&buffer);
	master->players()[0]->play_state(DataJockey::Player::PLAY);
	master->players()[0]->play_speed(0.95);

	master->players()[1]->audio_buffer(&buffer2);
	master->players()[1]->play_state(DataJockey::Player::PLAY);
	master->players()[1]->play_speed(1.1);

	cout << "starting" << endl;
	audioio.start();
	audioio.connectToPhysical(0,0);
	audioio.connectToPhysical(1,1);

	//audioio.connectToPhysical(2,0);
	//audioio.connectToPhysical(3,1);

	master->cross_fade(true);
	master->cross_fade_mixers(0,1);
	master->cross_fade_position(0);
	sleep(10);
	for(unsigned int i = 0; i <= 10; i++){
		master->cross_fade_position((float)i * 0.1);
		cout << master->cross_fade_position() << endl;
		sleep(1);
	}
	sleep(10);
	audioio.stop();
	sleep(1);

	return 0;
}

