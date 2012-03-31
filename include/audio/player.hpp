#ifndef DATAJOCKEY_PLAYER_HPP
#define DATAJOCKEY_PLAYER_HPP

#include "command.hpp"
#include "timepoint.hpp"
#include "transport.hpp"
#include "jackaudioio.hpp"
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"
#include "stretcher.hpp"
#include <slv2/collections.h>
#include <slv2/plugin.h>
#include <slv2/plugininstance.h>

namespace DataJockey {
   namespace Audio {
      struct EqControl {
         EqControl() : low(0.0), mid(0.0), high(0.0){}
         float low;
         float mid;
         float high;
         float latency;
      };
      class Player {
         public:
            //internal types
            enum play_state_t {PLAY, PAUSE};
            enum out_state_t {MAIN_MIX, CUE};
            enum eq_band_t {LOW, MID, HIGH};

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
            void audio_pre_compute(unsigned int numFrames, float ** mixBuffer,
                  const Transport& transport); 
            //actually compute one frame, filling an internal buffer
            //syncing to the transport if mSync == true,
            //inbeat reflects if the transport computed a new beat on the last
            //tick
            void audio_compute_frame(unsigned int frame, float ** mixBuffer, 
                  const Transport& transport, bool inbeat); 
            //finalize audio computation, apply effects, etc.
            void audio_post_compute(unsigned int numFrames, float ** mixBuffer); 
            //actually fill the output vectors
            void audio_fill_output_buffers(unsigned int numFrames, 
                  float ** mixBuffer, float ** cueBuffer);

            //getters
            play_state_t play_state() const;
            out_state_t out_state() const;
            bool muted() const;
            bool syncing() const;
            bool looping() const;
            double volume() const;
            double play_speed() const;
            const TimePoint& position() const;
            const TimePoint& start_position() const;
            const TimePoint& end_position() const;
            const TimePoint& loop_start_position() const;
            const TimePoint& loop_end_position() const;
            unsigned int frame() const;
            float max_sample_value() const;
            double bpm() const;

            AudioBuffer * audio_buffer() const;
            BeatBuffer * beat_buffer() const;

            //setters
            void play_state(play_state_t val);
            void out_state(out_state_t val);
            void mute(bool val);
            void sync(bool val);
            void loop(bool val);
            void volume(double val);
            void play_speed(double val);
            void position(const TimePoint &val);
            void position_at_frame(unsigned long frame);
            void start_position(const TimePoint &val);
            void end_position(const TimePoint &val);
            void loop_start_position(const TimePoint &val);
            void loop_end_position(const TimePoint &val);
            void audio_buffer(AudioBuffer * buf);
            void beat_buffer(BeatBuffer * buf);
            void eq(eq_band_t band, double value);
            void max_sample_value_reset();

            //misc
            void position_relative(TimePoint amt); //go to a position relative to the current position
            void position_at_frame_relative(long offset);
            void play_speed_relative(double amt); //increment or decrement the current play speed by amt
            void volume_relative(double amt); //increment or decrement the current volume

         private:
            //states
            play_state_t mPlayState;
            out_state_t mOutState;
            bool mCueMutesMain;
            bool mMute;
            bool mSync; //sync to main transport or not
            bool mLoop;
            bool mSetup;

            //continuous
            double mVolume;
            TimePoint mPosition; //the current position in the audio
            bool mPositionDirty; //indicates if the sampleindex needs update based
            //on the position update
            TimePoint mStartPosition; //where we start the playback
            TimePoint mEndPosition; //where we end the playback
            TimePoint mLoopStartPosition;
            TimePoint mLoopEndPosition;

            bool mUpdateTransportOffset;
            TimePoint mTransportOffset; //an offset to the transport, for syncing

            //internals, bookkeeping, etc
            unsigned int mSampleRate;
            float * mVolumeBuffer;
            BeatBuffer * mBeatBuffer;
            Stretcher * mStretcher;
            float mMaxSampleValue;

            //the eq instance
            SLV2Instance   mEqInstance;
            EqControl mEqControl;

            //helpers
            void update_position(const Transport& transport);
            //for updating the play speed while syncing
            void update_play_speed(const Transport& transport);
            void update_transport_offset(const Transport& transport);
      };

      class PlayerCommand : public Command {
         public:
            PlayerCommand(unsigned int idx);
            //getters
            unsigned int index() const;
            const TimePoint& position_executed() const;
            //setters
            void position_executed(TimePoint const & t);
            //misc
            void store(CommandIOData& data, const std::string& name) const;
         protected:
            Player * player();
         private:
            unsigned int mIndex;
            //this comes from the player's position
            TimePoint mPositionExecuted;
      };

      class PlayerStateCommand : public PlayerCommand {
         public:
            enum action_t {
               PLAY, PAUSE,
               OUT_MAIN, OUT_CUE,
               SYNC, NO_SYNC,
               MUTE, NO_MUTE,
               LOOP, NO_LOOP
            };
            PlayerStateCommand(unsigned int idx, action_t action);
            virtual void execute();
            virtual bool store(CommandIOData& data) const;
         private:
            action_t mAction;
      };

      class PlayerDoubleCommand : public PlayerCommand {
         public:
            enum action_t {
               VOLUME, VOLUME_RELATIVE,
               PLAY_SPEED, PLAY_SPEED_RELATIVE,
               EQ_LOW, EQ_MID, EQ_HIGH
            };
            PlayerDoubleCommand(unsigned int idx, action_t action, double value);
            virtual void execute();
            virtual bool store(CommandIOData& data) const;
         private:
            action_t mAction;
            double mValue;
      };

      class PlayerSetAudioBufferCommand : public PlayerCommand {
         public:
            //set the buffer, if you set deleteOldBuffer the buffer that is replaced with the given buffer
            //will be deleted during the execute_done action [or the destructor].  Otherwise, the old buffer
            //is up to you to deal with
            PlayerSetAudioBufferCommand(unsigned int idx, AudioBuffer * buffer, bool deleteOldBuffer = false);
            virtual ~PlayerSetAudioBufferCommand();
            virtual void execute();
            virtual void execute_done();
            virtual bool store(CommandIOData& data) const;
            AudioBuffer * buffer() const;
            void buffer(AudioBuffer * buffer);
         private:
            AudioBuffer * mBuffer;
            AudioBuffer * mOldBuffer;
            bool mDeleteOldBuffer;
      };

      class PlayerSetBeatBufferCommand : public PlayerCommand {
         public:
            //set the buffer, if you set deleteOldBuffer the buffer that is replaced with the given buffer
            //will be deleted during the execute_done action [or the destructor].  Otherwise, the old buffer
            //is up to you to deal with
            PlayerSetBeatBufferCommand(unsigned int idx, BeatBuffer * buffer, bool deleteOldBuffer = false);
            virtual ~PlayerSetBeatBufferCommand();
            virtual void execute();
            virtual void execute_done();
            virtual bool store(CommandIOData& data) const;
            BeatBuffer * buffer() const;
            void buffer(BeatBuffer * buffer);
         private:
            BeatBuffer * mBuffer;
            BeatBuffer * mOldBuffer;
            bool mDeleteOldBuffer;
      };

      class PlayerPositionCommand : public PlayerCommand {
         public:
            enum position_t {
               PLAY, PLAY_RELATIVE, 
               START, END, LOOP_START, LOOP_END
            };
            PlayerPositionCommand(unsigned int idx, position_t target, const TimePoint & timepoint);
            PlayerPositionCommand(unsigned int idx, position_t target, long frames);
            virtual void execute();
            virtual bool store(CommandIOData& data) const;
         private:
            TimePoint mTimePoint;
            position_t mTarget;
            long mFrames;
            bool mUseFrames;
      };
   }
}

#endif
