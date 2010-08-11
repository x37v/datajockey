#ifndef DATAJOCKEY_MIXER_HPP
#define DATAJOCKEY_MIXER_HPP

#include "player.hpp"
#include "transport.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include <vector>

namespace DataJockey {
	class Master {
		private:
			//singleton
			Master();
			Master(const Master&);
			Master& operator=(const Master&);
			~Master();
			static Master * cInstance;
		public:
			static Master * instance();
			//this creates internal buffers
			//** must be called BEFORE the audio callback starts 
			//but after all the players are added
			void setup_audio(
					unsigned int sampleRate,
					unsigned int maxBufferLen);
			//cannot be called while audio callback is running
			void add_player();
			//actually compute nframes of audio
			void audio_compute_and_fill(JackCpp::AudioIO::audioBufVector outBufferVector,
					unsigned int numFrames);

			//getters
			float master_volume() const;
			float cue_volume() const;
			bool cross_fadeing() const;
			float cross_fade_position() const;
			unsigned int cross_fade_mixer(unsigned int index) const;
			const std::vector<Player *>& players() const;
			Scheduler * scheduler();
			Transport * transport();

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
			Transport mTransport;
			Scheduler mScheduler;
			float mMasterVolume;
			float mCueVolume;
			unsigned int mCrossFadeMixers[2];
			bool mCrossFade;
			float mCrossFadePosition;
	};
	class MasterCommand : public Command {
		public:
			MasterCommand();
			Master * master() const;
		private:
			Master * mMaster; 
	};
	class MasterBoolCommand : public MasterCommand {
		public:
			enum action_t {
				XFADE, NO_XFADE
			};
			MasterBoolCommand(action_t action);
			virtual void execute();
			virtual bool store(CommandIOData& data);
		private:
			action_t mAction;
	};
	class MasterDoubleCommand : public MasterCommand {
		public:
			enum action_t {
				MAIN_VOLUME,
				CUE_VOLUME,
				XFADE_POSITION
			};
			MasterDoubleCommand(action_t action, double val);
			virtual void execute();
			virtual bool store(CommandIOData& data);
		private:
			action_t mAction;
			double mValue;
	};
	class MasterXFadeSelectCommand : public MasterCommand {
		public:
			MasterXFadeSelectCommand(unsigned int left, unsigned int right);
			virtual void execute();
			virtual bool store(CommandIOData& data);
		private:
			unsigned int mSel[2];
	};
}

#endif
