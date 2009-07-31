#ifndef DATAJOCKEY_PLAYER_HPP
#define DATAJOCKEY_PLAYER_HPP

#include "timepoint.hpp"
#include "transport.hpp"
#include "jackaudioio.hpp"
#include "audiobuffer.hpp"
#include "RubberBandStretcher.h"

namespace DataJockey {
	class Player {
		public:
			//internal types
			enum play_state_t {PLAY, PAUSE};
			enum out_state_t {MAIN_MIX, CUE};
			enum stretch_method_t {PLAY_RATE, RUBBER_BAND};

			Player();
			~Player();

			//this creates internal buffers
			//** must be called BEFORE the audio callback starts
			void setup_audio(
					unsigned int sampleRate,
					unsigned int maxBufferLen);

			//the audio computation methods
			//the player doesn't own its own buffer, it is passed it..
			//but it does own it between pre_compute and fill_output
			//setup for audio computation, we will be computing numFrames
			void audio_pre_compute(unsigned int numFrames, float ** mixBuffer); 
			//actually compute one frame, filling an internal buffer
			//syncing to the transport if mSync == true
			void audio_compute_frame(unsigned int frame, float ** mixBuffer, 
					const Transport * transport); 
			//finalize audio computation, apply effects, etc.
			void audio_post_compute(unsigned int numFrames, float ** mixBuffer); 
			//actually fill the output vectors
			void audio_fill_output_buffers(unsigned int numFrames, 
					float ** mixBuffer, float ** cueBuffer);

			//getters
			play_state_t play_state();
			out_state_t out_state();
			stretch_method_t stretch_method();
			bool muted();
			bool syncing();
			bool looping();
			double volume();
			double play_speed();
			TimePoint position();
			TimePoint start_position();
			TimePoint end_position();
			TimePoint loop_start_position();
			TimePoint loop_end_position();

			//setters
			void play_state(play_state_t val);
			void out_state(out_state_t val);
			void stretch_method(stretch_method_t val);
			void mute(bool val);
			void sync(bool val);
			void loop(bool val);
			void volume(double val);
			void play_speed(double val);
			void position(TimePoint val);
			void start_position(TimePoint val);
			void end_position(TimePoint val);
			void loop_start_position(TimePoint val);
			void loop_end_position(TimePoint val);

			//misc
			void position_relative(TimePoint amt); //go to a position relative to the current position
			void play_speed_relative(double amt); //increment or decrement the current play speed by amt
			void volume_relative(double amt); //increment or decrement the current volume

		private:
			//states
			play_state_t mPlayState;
			out_state_t mOutState;
			stretch_method_t mStretchMethod;
			bool mMute;
			bool mSync; //sync to main transport or not
			bool mLoop;

			//continuous
			double mVolume;
			double mPlaySpeed;
			TimePoint mPosition; //the current position in the audio
			TimePoint mStartPosition; //where we start the playback
			TimePoint mEndPosition; //where we end the playback
			TimePoint mLoopStartPosition;
			TimePoint mLoopEndPosition;

			//internals, bookkeeping, etc
			unsigned int mSampleRate;
			float * mVolumeBuffer;
			unsigned int mSampleIndex;
			double mSampleIndexResidual;
			RubberBand::RubberBandStretcher * mRubberBandStretcher;

		public:
			AudioBuffer * mAudioBuffer;
	};
}

#endif
