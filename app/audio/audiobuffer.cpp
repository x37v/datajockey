#include "audiobuffer.hpp"
#include <algorithm>

#define READ_FRAME_SIZE 32768

template<typename Type>
Type linear_interp(Type v0, Type v1, double dist){
  return v0 + (v1 - v0) * dist;
}

using namespace djaudio;

AudioBuffer::AudioBuffer(std::string soundfileLocation)
  throw(std::runtime_error) :
    mSoundFile(soundfileLocation.c_str()),
    mLoaded(false),
    mAbort(false),
    mNumChannels(0),
    mMaxSample(0.0),
    mNormalize(true)
{

  //check to make sure soundfile exists
  if(!mSoundFile.valid()){
    std::string str("cannot open soundfile: ");
    str.append(soundfileLocation);
    throw std::runtime_error(str);
  }

  mSampleRate = mSoundFile.samplerate();
  mNumChannels = mSoundFile.channels();
}

AudioBuffer::~AudioBuffer() {
}

//getters
unsigned int AudioBuffer::sample_rate() const{
  return mSampleRate;
}

unsigned int AudioBuffer::channels() const {
  return mNumChannels;
}

unsigned int AudioBuffer::length() const{
  unsigned int chans = channels();
  if (chans)
    return mAudioData.size() / chans;
  return 0;
}

bool AudioBuffer::loaded() const { return mLoaded; }

float AudioBuffer::sample(unsigned int channel, unsigned int index) const{
  index = channel + index * channels();
  //make sure we're in range
  if(mAudioData.size() <= index)
    return 0.0;
  else
    return mAudioData.at(index);
}

float AudioBuffer::sample(unsigned int channel, unsigned int index, double subsample) const {
  float sample0 = sample(channel, index);
  float sample1 = sample(channel, index + 1);
  return linear_interp(sample0, sample1, subsample);
}

QString AudioBuffer::file_location() const {
  return mSoundFile.location();
}

#include <iostream>
using std::cerr;
using std::endl;

void AudioBuffer::fill_mono(data_buffer_t& buffer, unsigned int start_index) const {
  const unsigned int num_channels = channels();
  if (num_channels == 0) {
    cerr << "num_channels == 0" << endl;
  }
  const unsigned int frames = buffer.size();
  const unsigned int frames_total = mAudioData.size() / num_channels;
  const unsigned int valid_frames = ((frames + start_index) <= frames_total) ? frames : frames_total - start_index;

  unsigned int i = 0;
  float mult = 1.0f / (float)num_channels;
  for (; i < valid_frames; i++) {
    buffer[i] = 0.0f;
    for (unsigned int j = 0; j < num_channels; j++)
      buffer[i] += mAudioData[(start_index + i) * num_channels + j];
    buffer[i] *= mult;
  }
  //zero pad
  for (; i < frames; i++)
    buffer[i] = 0.0f;
}

bool AudioBuffer::valid() const {
  return loaded() && mSoundFile.valid();
}

bool AudioBuffer::load(progress_callback_t progress_callback, void * user_data) {
  mAbort = false;
  //if it is loaded then simply report that and return
  if (mLoaded) {
    if (progress_callback)
      progress_callback(100, user_data);
    return true;
  }

  float * inbuf = NULL;
  unsigned int frames_read;
  unsigned int chans;

  //read in the audio data
  inbuf = new float[READ_FRAME_SIZE * channels()];
  chans = channels();
  double num_frames = (double)mSoundFile.frames();
  unsigned int total_read = 0;
  unsigned int percent_last = 0;

  if (progress_callback)
    progress_callback(0, user_data);

  while(!mAbort && (frames_read = mSoundFile.readf(inbuf, READ_FRAME_SIZE)) != 0){
    for(unsigned int i = 0; i < frames_read; i++){
      for(unsigned int j = 0; j < chans; j++){
        float v = inbuf[i * chans + j];
        mAudioData.push_back(v);
        //find the max sample for normalization
        mMaxSample = std::max(v, mMaxSample);
      }
    }

    //report progress
    if (progress_callback && num_frames != 0) {
      total_read += frames_read;
      unsigned int new_percent = (double)(100 * total_read) / num_frames;
      if (new_percent != percent_last) {
        percent_last = new_percent;
        progress_callback(percent_last, user_data);
      }
    }
  }
  delete [] inbuf;
  if (!mAbort) {
    if (mNormalize && mMaxSample > 0.0 && mMaxSample < 1.0) {
      float mul = 1.0 / mMaxSample;
      for (unsigned int i = 0; i < mAudioData.size(); i++)
        mAudioData[i] *= mul;
    }

    mLoaded = true;
    if (progress_callback)
      progress_callback(100, user_data);
    return true;
  }
  return false;
}

void AudioBuffer::abort_load(){ mAbort = true; }

const AudioBuffer::data_buffer_t& AudioBuffer::raw_buffer() const {
  return mAudioData;
}

