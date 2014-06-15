#ifndef DATAJOCKEY_MIXER_HPP
#define DATAJOCKEY_MIXER_HPP

#include "player.hpp"
#include "transport.hpp"
#include "scheduler.hpp"
#include "types.hpp"
#include <vector>

#ifdef USE_LV2
#include <lilv/lilv.h>
#endif

namespace djaudio {
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
      Player * add_player();
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
      float max_sample_value() const;
      bool player_audible(unsigned int player_index) const;

#ifdef USE_LV2
      LilvWorld * lv2_world() const;
      const LilvPlugins * lv2_plugins() const;
#endif

      //setters
      void master_volume(float val);
      void cue_volume(float val);
      void cross_fade(bool val);
      void cross_fade_position(float val);
      void cross_fade_mixers(unsigned int left, unsigned int right);
      void sync_to_player(unsigned int player_index);
      float max_sample_value_reset();
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
      float mMaxSampleValue;

#ifdef USE_LV2
      LilvWorld * mLV2World;
      const LilvPlugins * mLV2Plugins;
#endif
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
      virtual bool store(CommandIOData& data) const;
    private:
      action_t mAction;
  };
  class MasterIntCommand : public MasterCommand {
    public:
      enum action_t {
        SYNC_TO_PLAYER
      };
      MasterIntCommand(action_t action, int value);
      virtual void execute();
      virtual bool store(CommandIOData& data) const;
    private:
      action_t mAction;
      int mValue;
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
      virtual bool store(CommandIOData& data) const;
    private:
      action_t mAction;
      double mValue;
  };
  class MasterXFadeSelectCommand : public MasterCommand {
    public:
      MasterXFadeSelectCommand(unsigned int left, unsigned int right);
      virtual void execute();
      virtual bool store(CommandIOData& data) const;
    private:
      unsigned int mSel[2];
  };
}

#endif