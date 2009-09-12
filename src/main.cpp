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
	DataJockey::AudioIO * audioio = DataJockey::AudioIO::instance();
	cout << "reading in buffers" << endl;
	//DataJockey::AudioBuffer buffer2("/mp3/model_500/classics/02-the_chase_smooth_remix.flac");
	//DataJockey::BeatBuffer beatBuffer2("/home/alex/.datajockey/annotation/2.yaml");
	//DataJockey::AudioBuffer buffer("/mp3/new_order/movement/01-dreams_never_end.flac");
	//DataJockey::BeatBuffer beatBuffer("/home/alex/.datajockey/annotation/2186.yaml");
	DataJockey::AudioBuffer buffer("/mp3/compilations/street_jams_electric_funk/part_2/09-jive_rhythm_trax-122_b_p_m_.flac");
	DataJockey::BeatBuffer beatBuffer("/home/alex/music/annotation/377.yaml");
	DataJockey::AudioBuffer buffer2("/mp3/2_live_crew/is_what_we_are/5-throw_the_d.flac");
	DataJockey::BeatBuffer beatBuffer2("/home/alex/music/annotation/441.yaml");
	DataJockey::Master * master = DataJockey::Master::instance();
	master->add_player();
	master->add_player();

	master->players()[0]->audio_buffer(&buffer);
	master->players()[0]->beat_buffer(&beatBuffer);
	master->players()[0]->play_state(DataJockey::Player::PLAY);
	master->players()[0]->play_speed(0.95);
	master->players()[0]->sync(true);


	master->players()[1]->audio_buffer(&buffer2);
	master->players()[1]->beat_buffer(&beatBuffer2);
	master->players()[1]->play_state(DataJockey::Player::PLAY);
	master->players()[1]->play_speed(1.1);
	master->players()[1]->sync(true);

	cout << "starting" << endl;
	audioio->start();
	audioio->connectToPhysical(0,0);
	audioio->connectToPhysical(1,1);
	{
		using namespace DataJockey;
		TimePoint pos;
		pos.at_bar(10);
		//cout << "ending loop" << endl;
		//this is pointless but I just want to make sure it works
		master->scheduler()->remove(
				master->scheduler()->schedule( pos,
					new DataJockey::PlayerStateCommand(1,
						DataJockey::PlayerStateCommand::NO_LOOP)
					));
		//actually schedule the command
		master->scheduler()->schedule( pos,
				new DataJockey::PlayerStateCommand(1,
					DataJockey::PlayerStateCommand::NO_LOOP)
				);
	}

	{
		using namespace DataJockey;
		TimePoint pos;
		unsigned int i;
		for(i = 0; i < 16; i++){
			pos.at_bar(i / 4, i % 4);
			master->scheduler()->schedule( pos,
					new DataJockey::PlayerStateCommand(i % 2, 
						DataJockey::PlayerStateCommand::MUTE)
					);
			master->scheduler()->schedule( pos,
					new DataJockey::PlayerStateCommand((i + 1) % 2, 
						DataJockey::PlayerStateCommand::NO_MUTE)
					);
		}
		pos.at_bar(i / 4, i % 4);
		master->scheduler()->schedule( pos,
				new DataJockey::PlayerStateCommand(i % 2, 
					DataJockey::PlayerStateCommand::NO_MUTE)
				);
		master->scheduler()->schedule( pos,
				new DataJockey::PlayerStateCommand((i + 1) % 2, 
					DataJockey::PlayerStateCommand::NO_MUTE)
				);
	}

	{
		using namespace DataJockey;
		TimePoint pos;

		//player 0
		pos.at_bar(1, 0);
		master->scheduler()->execute(
				new DataJockey::PlayerPositionCommand(0,
					PlayerPositionCommand::PLAY,
					pos)
				);

		//player 1
		pos.at_bar(1, 1);
		master->scheduler()->execute(
				new DataJockey::PlayerPositionCommand(1,
					PlayerPositionCommand::LOOP_START,
					pos)
				);
		master->scheduler()->execute(
				new DataJockey::PlayerPositionCommand(1,
					PlayerPositionCommand::PLAY,
					pos)
				);
		pos.at_bar(3, 1);
		master->scheduler()->execute(
				new DataJockey::PlayerPositionCommand(1,
					PlayerPositionCommand::LOOP_END,
					pos)
				);
		master->scheduler()->execute(
				new DataJockey::PlayerStateCommand(1,
					PlayerStateCommand::LOOP)
				);
	}

	sleep(1);
	master->scheduler()->execute_done_actions();

	//audioio.connectToPhysical(2,0);
	//audioio.connectToPhysical(3,1);
	
	{
		using namespace DataJockey;
		//master->cross_fade(true);
		master->scheduler()->execute( new MasterBoolCommand(MasterBoolCommand::XFADE));
		//master->cross_fade_mixers(0,1);
		master->scheduler()->execute( new MasterXFadeSelectCommand(0,1));
		//master->cross_fade_position(0.5);
		master->scheduler()->execute( new MasterDoubleCommand(
					MasterDoubleCommand::XFADE_POSITION, 0.5));
		sleep(20);
		for(unsigned int i = 5; i <= 10; i++){
			//master->cross_fade_position((float)i * 0.1);
			master->scheduler()->execute( new MasterDoubleCommand(
						MasterDoubleCommand::XFADE_POSITION, (float)i * 0.1));
			sleep(1);
			cout << "xfade pos: " << master->cross_fade_position() << endl;
		}
		for(unsigned int i = 0; i < 20; i++){
			master->scheduler()->execute( new TransportBPMCommand(master->transport(), 
						master->transport()->bpm() + 0.5));
			usleep(100000);
			cout << "bpm: " << master->transport()->bpm() << endl;
		}
	}

	sleep(1);
	{
		cout << "ending sync" << endl;
		master->scheduler()->execute(
				new DataJockey::PlayerStateCommand(1,
					DataJockey::PlayerStateCommand::NO_SYNC)
				);
	}

	sleep(1);
	master->scheduler()->execute_done_actions();
	sleep(10);
	cout << "stopping" << endl;
	audioio->stop();
	sleep(1);

	return 0;
}

