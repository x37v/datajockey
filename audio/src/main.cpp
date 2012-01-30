#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "audioio.hpp"
#include "player.hpp"
#include "master.hpp"
#include "timepoint.hpp"
#include <iostream>

using std::cout;
using std::endl;

double beats[] = {
   0.726959521030585,
   1.22441273627421,
   1.7224128908348,
   2.22076340426257,
   2.71918287925675,
   3.21749599330739,
   3.71572882337147,
   4.21404419133376,
   4.7125842649004,
   5.21134850169258,
   5.71018300787509,
   6.20888400511536,
   6.70734962913104,
   7.20566024875033,
   7.70401329643411,
   8.20257082211528,
   8.70134283173138,
   9.20018068225294,
   9.69888308449745,
   10.1973494896166,
   10.695662550328,
   11.1940296055987,
   11.6926464528295,
   12.1915992759135,
   12.6908428503628,
   13.1902209108979,
   13.6895107425958,
   14.1885107408087,
   14.6871549411674,
   15.1855662026371,
   15.6839842696934,
   16.1826280095256,
   16.6816043648998,
   17.180900531162,
   17.6804142684115,
   18.1799773431669,
   18.6793810854619,
   19.1784455060204,
   19.6771236875743,
   20.1755518470633,
   20.6739777135131,
   21.172623072077,
   21.6715891045401,
   22.1708385617665,
   22.67021915268,
   23.1695100397511,
   23.6685104661985,
   24.1671548265437,
   24.6655659390649,
   25.1639818143221,
   25.6626116427624,
   26.1615287129784,
   26.6606440792932,
   27.1597520975557,
   27.6586395062619,
   28.1572196080323,
   28.6555948746135,
   29.1539819904911,
   29.6525564567503,
   30.1513364831049,
   30.6501779685233,
   31.1488819489544,
   31.6473488105853,
   32.1456599300929,
   32.6440131748812,
   33.1425707766033,
   33.6413428149792,
   34.1401806761829,
   34.6388830727929,
   35.1373492647393,
   35.635660109252,
   36.1340132440322,
   36.6325708027695,
   37.1313428247024,
   37.6301806701996,
   38.1288828594952,
   38.6273470389597,
   39.1256459741695,
   39.6239517292999,
   40.122375866375,
   40.6208755904991,
   41.1193100771233,
   41.6175921992855,
   42.1157895181196,
   42.6140783835182,
   43.1126020204783,
   43.6113571574644,
   44.1101870069712,
   44.6088855689776,
   45.1073481634651,
   45.6056464285637,
   46.1039519085442,
   46.6023759355559,
   47.1008756071388,
   47.5993098722733,
   48.0975899766068,
   48.5957753841575,
   49.0940168691849,
   49.592407084224,
   50.0908899233098,
   50.5893164139116,
   51.0875949087737,
   51.5857906426269,
   52.084078837913,
   52.5826021997228,
   53.0813572266454,
   53.5801870426844,
   54.0788857932811,
   54.5773503932533,
   55.0756605650845,
   55.5740134237856,
   56.0725708721283,
   56.5713428414035,
   57.0701804653706,
   57.5688806368235,
   58.067332905,
   58.5655844598369,
   59.0637567930459,
   59.5619086322205,
   60.0600049879028,
   60.5580192023426,
   61.0560324522139,
   61.5542077922275,
   62.0526671599095,
   62.5513883751544,
   63.0502013397269,
   63.5488919057471,
   64.0473508729468,
   64.5456475530688,
   65.0439523629383,
   65.5423761148002,
   66.0408756763198,
   66.5393098984498,
   67.0375899863334,
   67.5357753877122,
   68.0340168704643,
   68.532407084678,
   69.0308899139321,
   69.5293161993899,
   70.027592682559,
   70.5257765073918,
   71.0240173231278,
   71.5224072633102,
   72.020889992436,
   72.5193164400693,
   73.017594918494,
   73.5157906461795,
   74.0140788391917,
   74.5126022001766,
   75.0113572268044,
   75.5101870427394,
   76.0088857932999,
   76.5073503932596,
   77.0056605650867,
   77.5040134237863,
   78.0025708721285,
   78.5013428509403,
   79.0001806799473
};

using namespace DataJockey::Audio;
int main(int argc, char * argv[]){
   AudioIO * audioio = DataJockey::Audio::AudioIO::instance();
   Master * master = DataJockey::Audio::Master::instance();

   cout << "reading in buffer" << endl;
   AudioBuffer audio_buffer("/home/alex/projects/music/11-phuture-acid_tracks.flac");
   audio_buffer.load();
   BeatBuffer beat_buffer;
   for(unsigned int i = 0; i < 150; i++) {
      beat_buffer.insert_beat(beats[i]);
   }
   master->add_player();

   //master->players()[0]->audio_buffer(&audio_buffer);
   //master->players()[0]->beat_buffer(&beat_buffer);
   master->players()[0]->play_state(DataJockey::Audio::Player::PLAY);
   master->players()[0]->out_state(DataJockey::Audio::Player::MAIN_MIX);
   master->players()[0]->sync(false);

   cout << "starting" << endl;
   audioio->start();
   audioio->connectToPhysical(0,0);
   audioio->connectToPhysical(1,1);

   sleep(2);
   master->scheduler()->execute(new PlayerSetAudioBufferCommand(0, &audio_buffer, false));
   master->scheduler()->execute(new PlayerSetBeatBufferCommand(0, &beat_buffer, false));
   master->scheduler()->execute(new PlayerStateCommand(0, PlayerStateCommand::SYNC));
   master->scheduler()->execute(new TransportBPMCommand(master->transport(), 140.0));
   //master->transport()->bpm(100.0);

   for(unsigned int i = 0; i < 20000; i++) {
      usleep(1000);
      master->scheduler()->execute_done_actions();
   }
   cout << "stopping" << endl;
   audioio->stop();
   exit(0);
}

#if 0
int main(int argc, char * argv[]){
   if(argc < 3)
      return -1;
   DataJockey::Audio::AudioIO * audioio = DataJockey::Audio::AudioIO::instance();
   cout << "reading in buffers" << endl;
   //DataJockey::Audio::AudioBuffer buffer2("/mp3/model_500/classics/02-the_chase_smooth_remix.flac");
   //DataJockey::Audio::BeatBuffer beatBuffer2("/home/alex/.datajockey/annotation/2.yaml");
   //DataJockey::Audio::AudioBuffer buffer("/mp3/new_order/movement/01-dreams_never_end.flac");
   //DataJockey::Audio::BeatBuffer beatBuffer("/home/alex/.datajockey/annotation/2186.yaml");
   DataJockey::Audio::AudioBuffer buffer(argv[1]);
   //DataJockey::Audio::BeatBuffer beatBuffer("/home/alex/music/annotation/377.yaml");
   DataJockey::Audio::AudioBuffer buffer2(argv[2]);
   //DataJockey::Audio::BeatBuffer beatBuffer2("/home/alex/music/annotation/441.yaml");
   DataJockey::Audio::Master * master = DataJockey::Audio::Master::instance();
   master->add_player();
   master->add_player();

   //master->players()[0]->audio_buffer(&buffer);
   //master->players()[0]->beat_buffer(&beatBuffer);
   master->players()[0]->play_state(DataJockey::Audio::Player::PLAY);
   master->players()[0]->out_state(DataJockey::Audio::Player::MAIN_MIX);
   master->players()[0]->play_speed(0.95);
   master->players()[0]->sync(true);


   //master->players()[1]->audio_buffer(&buffer2);
   //master->players()[1]->beat_buffer(&beatBuffer2);
   master->players()[1]->play_state(DataJockey::Audio::Player::PLAY);
   master->players()[1]->play_speed(1.5);
   master->players()[1]->out_state(DataJockey::Audio::Player::MAIN_MIX);
   master->players()[1]->sync(true);

   cout << "starting" << endl;
   audioio->start();
   audioio->connectToPhysical(0,0);
   audioio->connectToPhysical(1,1);

   sleep(20);
   exit(0);

   {
      DataJockey::Audio::TimePoint pos;
      pos.at_bar(10);
      //cout << "ending loop" << endl;
      //this is pointless but I just want to make sure it works
      master->scheduler()->remove(
            master->scheduler()->schedule( pos,
               new DataJockey::Audio::PlayerStateCommand(1,
                  DataJockey::Audio::PlayerStateCommand::NO_LOOP)
               ));
      //actually schedule the command
      master->scheduler()->schedule( pos,
            new DataJockey::Audio::PlayerStateCommand(1,
               DataJockey::Audio::PlayerStateCommand::NO_LOOP)
            );
   }

   {
      DataJockey::Audio::TimePoint pos;
      unsigned int i;
      for(i = 0; i < 16; i++){
         pos.at_bar(i / 4, i % 4);
         master->scheduler()->schedule( pos,
               new DataJockey::Audio::PlayerStateCommand(i % 2, 
                  DataJockey::Audio::PlayerStateCommand::MUTE)
               );
         master->scheduler()->schedule( pos,
               new DataJockey::Audio::PlayerStateCommand((i + 1) % 2, 
                  DataJockey::Audio::PlayerStateCommand::NO_MUTE)
               );
      }
      pos.at_bar(i / 4, i % 4);
      master->scheduler()->schedule( pos,
            new DataJockey::Audio::PlayerStateCommand(i % 2, 
               DataJockey::Audio::PlayerStateCommand::NO_MUTE)
            );
      master->scheduler()->schedule( pos,
            new DataJockey::Audio::PlayerStateCommand((i + 1) % 2, 
               DataJockey::Audio::PlayerStateCommand::NO_MUTE)
            );
   }

   {
      DataJockey::Audio::TimePoint pos;

      //player 0
      pos.at_bar(1, 0);
      master->scheduler()->execute(
            new DataJockey::Audio::PlayerPositionCommand(0,
               DataJockey::Audio::PlayerPositionCommand::PLAY,
               pos)
            );

      //player 1
      pos.at_bar(1, 1);
      master->scheduler()->execute(
            new DataJockey::Audio::PlayerPositionCommand(1,
               DataJockey::Audio::PlayerPositionCommand::LOOP_START,
               pos)
            );
      master->scheduler()->execute(
            new DataJockey::Audio::PlayerPositionCommand(1,
               DataJockey::Audio::PlayerPositionCommand::PLAY,
               pos)
            );
      pos.at_bar(3, 1);
      master->scheduler()->execute(
            new DataJockey::Audio::PlayerPositionCommand(1,
               DataJockey::Audio::PlayerPositionCommand::LOOP_END,
               pos)
            );
      master->scheduler()->execute(
            new DataJockey::Audio::PlayerStateCommand(1,
               DataJockey::Audio::PlayerStateCommand::LOOP)
            );
   }

   sleep(1);
   master->scheduler()->execute_done_actions();

   //audioio.connectToPhysical(2,0);
   //audioio.connectToPhysical(3,1);

   {
      using namespace DataJockey::Audio;
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
            new DataJockey::Audio::PlayerStateCommand(1,
               DataJockey::Audio::PlayerStateCommand::NO_SYNC)
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
#endif

