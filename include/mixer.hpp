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
			bool cross_fadeing();
			float cross_fade_position();
			unsigned int cross_fade_mixer(unsigned int index);

			//setters
			void master_volume(float val);
			void cue_volume(float val);
			void cross_fade(bool val);
			void cross_fade_position(float val);
			void cross_fade_mixers(unsigned int left, unsigned int right);
		private:
			//internal buffers
			std::vector<float **> mPlayerBuffers;
			float ** mCueBuffer;
			float * mMasterVolumeBuffer;
			float ** mCrossFadeBuffer;

			std::vector<Player *> mPlayers;
			float mMasterVolume;
			float mCueVolume;
			unsigned int mCrossFadeMixers[2];
			bool mCrossFade;
			float mCrossFadePosition;
	};
}

#endif
