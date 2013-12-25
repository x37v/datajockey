#include "stretcher.hpp"

namespace djaudio {
  Stretcher::Stretcher() :
    mFrame(0),
    mFrameSubsample(0.0),
    mSpeed(1.0),
    mAudioBuffer(NULL) {
    }

  Stretcher::~Stretcher() { }

  void Stretcher::audio_buffer(AudioBuffer * buffer) {
    mAudioBuffer = buffer;
    mFrame = 0;
    mFrameSubsample = 0.0;
    audio_changed();
    frame_updated();
  }

  AudioBuffer * Stretcher::audio_buffer() const { return mAudioBuffer; }

  void Stretcher::reset() {
    mSpeed = 0.0;
    audio_buffer(NULL);
    speed_updated();
  }

  //set the frame
  void Stretcher::frame(unsigned int frame, double frame_subsamp) {
    if (!mAudioBuffer)
      return;

    mFrame = frame;
    mFrameSubsample = frame_subsamp;

    if (mFrame >= mAudioBuffer->length()) {
      mFrame = mAudioBuffer->length() - 1;
      mFrameSubsample = 0.0;
    }

    if (mFrameSubsample < 0.0)
      mFrameSubsample = 0.0;

    frame_updated();
  }

  unsigned int Stretcher::frame() const { return mFrame; }
  double Stretcher::frame_subsample() const { return mFrameSubsample; }
  double Stretcher::seconds() const {
    if (!mAudioBuffer)
      return 0.0;
    return static_cast<double>(mFrame) / static_cast<double>(mAudioBuffer->sample_rate());
  }

  void Stretcher::seconds(double seconds) {
    if (!mAudioBuffer)
      return;
    unsigned int f = static_cast<unsigned int>(seconds * static_cast<double>(mAudioBuffer->sample_rate()));
    frame(f);
  }

  //set the playback speed
  void Stretcher::speed(double play_speed) {
    mSpeed = play_speed;
    speed_updated();
  }

  double Stretcher::speed() const { return mSpeed; }

  void Stretcher::next_frame(float * frame) {
    if (!mAudioBuffer) {
      frame[0] = frame[1] = 0.0;
      return;
    }

    double last_frame_subsamp = mFrameSubsample;
    unsigned int last_frame = mFrame;

    //update our indices
    mFrameSubsample += speed();
    while (mFrameSubsample >= 1.0) {
      mFrame += 1;
      mFrameSubsample -= 1.0;
    }

    while (mFrameSubsample < 0.0) {
      if (mFrame == 0) {
        mFrameSubsample = 0.0;
        break;
      }
      mFrame -= 1;
      mFrameSubsample += 1.0;
    }

    //TODO should we special case the last sample and set mFrameSubsample to zero?
    if (mFrame >= (mAudioBuffer->length() - 1)) {
      mFrame = mAudioBuffer->length();
      mFrameSubsample = 0.0;
      frame[0] = frame[1] = 0.0;
      return;
    }

    compute_frame(frame, mFrame, mFrameSubsample, last_frame, last_frame_subsamp);
  }

  void Stretcher::next(float * frame_buffer, unsigned int frames) {
    if (!mAudioBuffer)
      return;

    const unsigned int channels = mAudioBuffer->channels();
    for (unsigned int i = 0; i < frames * channels; i += channels)
      next_frame(frame_buffer + i);
  }

  bool Stretcher::pitch_independent() const { return false; }


  void Stretcher::audio_changed() { }
  void Stretcher::frame_updated() { }
  void Stretcher::speed_updated() { }
}

