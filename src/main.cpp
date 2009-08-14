#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "audioio.hpp"
#include "player.hpp"
#include "master.hpp"
#include "timepoint.hpp"
#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char * argv[]){
	//if(argc < 3)
		//return -1;
	DataJockey::AudioIO audioio;
	cout << "reading in buffers" << endl;
	DataJockey::AudioBuffer buffer2("/mp3/model_500/classics/02-the_chase_smooth_remix.flac");
	DataJockey::BeatBuffer beatBuffer2("/home/alex/.datajockey/annotation/2.yaml");
	DataJockey::AudioBuffer buffer("/mp3/new_order/movement/01-dreams_never_end.flac");
	DataJockey::BeatBuffer beatBuffer("/home/alex/.datajockey/annotation/2186.yaml");
	DataJockey::Master * master = audioio.master();
	master->add_player();
	master->add_player();

	master->players()[0]->audio_buffer(&buffer);
	master->players()[0]->beat_buffer(&beatBuffer);
	master->players()[0]->play_state(DataJockey::Player::PLAY);
	//master->players()[0]->play_speed(0.95);
	master->players()[0]->sync(true);


	master->players()[1]->audio_buffer(&buffer2);
	master->players()[1]->beat_buffer(&beatBuffer2);
	master->players()[1]->play_state(DataJockey::Player::PLAY);
	//master->players()[1]->play_speed(1.1);
	master->players()[1]->sync(true);
	
	DataJockey::TimePoint pos;
	pos.at_bar(20, 0);
	master->players()[0]->position(pos);

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
	sleep(1);
	cout << "ending sync" << endl;
	master->players()[1]->sync(false);
	sleep(20);
	cout << "stopping" << endl;
	audioio.stop();
	sleep(1);

	return 0;
}

