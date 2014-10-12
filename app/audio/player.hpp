#ifndef DATAJOCKEY_PLAYER_HPP
#define DATAJOCKEY_PLAYER_HPP

#include "command.hpp"
#include "transport.hpp"
#include "jackaudioio.hpp"
#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "stretcher.hpp"
#include "envelope.hpp"

#ifdef USE_LV2
#include "lv2plugin.h"
#endif

namespace djaudio {
  class Player {
    public:
      //internal types
      enum play_state_t {PLAY, PAUSE};
      enum out_state_t {MAIN_MIX, CUE};
      enum eq_band_t {LOW = 0, MID = 1, HIGH = 2};

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
      unsigned int frame() const;
      unsigned int beat_index() const;
      float max_sample_value() const;
      double bpm();
      double pos_in_beat() const;
      bool audible() const;

      AudioBuffer * audio_buffer() const;
      BeatBuffer * beat_buffer() const;

      //setters
      void play_state(play_state_t val, Transport * transport = NULL);
      void out_state(out_state_t val);
      void mute(bool val);
      void sync(bool val, const Transport * transport = NULL);
      void loop(bool val);
      void volume(double val);
      void play_speed(double val);
      void position_at_frame(unsigned long frame, Transport * transport = NULL);
      void position_at_beat(unsigned int beat, Transport * transport = NULL);
      void position_at_beat_relative(int offset, Transport * transport = NULL);

      void loop_start_frame(unsigned int val);
      unsigned int loop_start_frame() const { return mLoopStartFrame; }

      void loop_end_frame(unsigned int val);
      unsigned int loop_end_frame() const { return mLoopEndFrame; }

      void audio_buffer(AudioBuffer * buf);
      void beat_buffer(BeatBuffer * buf);
      void eq(eq_band_t band, double value);
      float max_sample_value_reset();

      //misc
      void position_at_frame_relative(long offset);
      void play_speed_relative(double amt); //increment or decrement the current play speed by amt
      void volume_relative(double amt); //increment or decrement the current volume

    private:
      //states
      unsigned int mBeatIndex;
      play_state_t mPlayState;
      out_state_t mOutState;
      bool mCueMutesMain;
      bool mMute;
      bool mSync; //sync to main transport or not
      bool mLoop;
      bool mSetup;

      //continuous
      double mVolume;

      unsigned int mLoopStartFrame;
      unsigned int mLoopEndFrame;

      //internals, bookkeeping, etc
      unsigned int mSampleRate;
      float * mVolumeBuffer;
      BeatBuffer * mBeatBuffer;
      Stretcher * mStretcher;
      float mMaxSampleValue;

      Envelope mEnvelope;

      unsigned int mFadeoutIndex;
      std::vector<float> mFadeoutBuffer;

      //the eq instance
#ifdef USE_LV2
      Lv2Plugin * mEqPlugin;
#endif

      //helpers
      //for updating the play speed while syncing
      void update_play_speed(const Transport * transport);
      void sync_to_transport(const Transport * transport);
      double pos_in_beat(int pos_frame, unsigned int pos_beat) const;
      void fill_fade_buffer(); //moves our stretcher index..
      void setup_seek_fade();
  };

  //forward declaration
  class Master;

  class PlayerCommand : public Command {
    public:
      PlayerCommand(unsigned int idx);
      //getters
      unsigned int index() const;
      unsigned int beat_executed() const;
      //setters
      void beat_executed(unsigned int beat);
      //misc
      void store(CommandIOData& data, const std::string& name) const;
    protected:
      Player * player();
      Master * master() const { return mMaster; }
    private:
      unsigned int mIndex;
      //this comes from the player
      unsigned int mBeatExecuted;
      Master * mMaster;
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
      virtual void execute(const Transport& transport);
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
      virtual void execute(const Transport& transport);
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
      virtual void execute(const Transport& transport);
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
      virtual void execute(const Transport& transport);
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
        PLAY_BEAT, PLAY_BEAT_RELATIVE,
        LOOP_START, LOOP_END
      };
      PlayerPositionCommand(unsigned int idx, position_t target, int value);
      virtual void execute(const Transport& transport);
      virtual bool store(CommandIOData& data) const;
    private:
      position_t mTarget;
      int mValue;
  };

  class PlayerLoopCommand : public PlayerCommand {
    public:
      typedef enum {
        RESIZE_FROM_FRONT,
        RESIZE_FROM_BACK,
        RESIZE_AT_POSITION
      } resize_policy_t;
      explicit PlayerLoopCommand(unsigned int idx, double beats, resize_policy_t resize_policy = RESIZE_FROM_FRONT, bool start_looping = true);
      explicit PlayerLoopCommand(unsigned int idx, long start_frame, long end_frame, bool start_looping = true);
      virtual void execute(const Transport& transport);
      virtual bool store(CommandIOData& data) const;
      long start_frame() const { return mStartFrame; }
      long end_frame() const { return mEndFrame; }
      bool looping() const { return mLooping; }
    private:
      resize_policy_t mResizePolicy = RESIZE_FROM_FRONT;
      bool mStartLooping = true;
      bool mLooping = true;
      double mBeats = 0;
      long mStartFrame = -1; //less than zero implies that we need to compute it
      long mEndFrame = -1; //if they are both less than zero, we compute from current location
  };

  class PlayerLoopShiftCommand : public PlayerCommand {
    public:
      PlayerLoopShiftCommand(unsigned int idx, int beats);
      virtual void execute(const Transport& transport);
      virtual bool store(CommandIOData& data) const;

      long start_frame() const { return mStartFrame; }
      long end_frame() const { return mEndFrame; }
      bool looping() const { return mLooping; }
    private:
      bool mLooping = true;
      int mBeats = 0;
      long mStartFrame = -1;
      long mEndFrame = -1;
  };
}

#endif
