#ifndef DATAJOCKEY_MIXER_HPP
#define DATAJOCKEY_MIXER_HPP

#include "player.hpp"
#include <vector>

namespace DataJockey {
	class Mixer {
		public:
			Mixer(unsigned int numPlayers = 0);
			~Mixer();
			//this creates internal buffers
			//** must be called BEFORE the audio callback starts 
			//but after all the players are added
			void setup_audio(
					unsigned int sampleRate,
					unsigned int maxBufferLen);
			//cannot be called while audio callback is running
			void add_player(Player * p);
			//actually compute nframes of audio
			void audio_compute_and_fill(JackCpp::AudioIO::audioBufVector outBufferVector,
					unsigned int numFrames);

			//getters
			float master_volume();
			float cue_volume();

			//setters
			void master_volume(float val);
			void cue_volume(float val);
		private:
			//internal buffers
			std::vector<float **> mPlayerBuffers;
			float ** mCueBuffer;
			float * mMasterVolumeBuffer;

			std::vector<Player *> mPlayers;
			float mMasterVolume;
			float mCueVolume;
	};
}

#endif
