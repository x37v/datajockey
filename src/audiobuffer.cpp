#include "audiobuffer.hpp"

#define READ_FRAME_SIZE 2048

template<typename Type>
Type linear_interp(Type v0, Type v1, double dist){
	return v0 + (v1 - v0) * dist;
}

using namespace DataJockey;

AudioBuffer::AudioBuffer(std::string soundfileLocation)
   throw(std::runtime_error) :
   mSoundFile(soundfileLocation.c_str()),
   mLoaded(false),
   mAbort(false)
{
   //check to make sure soundfile exists
   if(!mSoundFile){
      std::string str("cannot open soundfile: ");
      str.append(soundfileLocation);
      throw std::runtime_error(str);
   }
	mAudioData.clear();
	mSampleRate = mSoundFile.samplerate();
	//we must resize the audio buffer because we want to make sure that
	//we can do mAudioData[i].push_back
	mAudioData.resize(mSoundFile.channels());
}

//getters
unsigned int AudioBuffer::sample_rate(){
	return mSampleRate;
}

unsigned int AudioBuffer::channels(){
	return mAudioData.size();
}

unsigned int AudioBuffer::length(){
	if(mAudioData.size() > 0)
		return mAudioData[0].size();
	else
		return 0;
}

bool AudioBuffer::loaded() { return mLoaded; }

float AudioBuffer::sample(unsigned int channel, unsigned int index){
	//make sure we're in range
	if(mAudioData.size() <= channel)
		return 0.0;
	else if(mAudioData[channel].size() <= index)
		return 0.0;
	else
		return mAudioData[channel][index];
}

float AudioBuffer::sample(unsigned int channel, unsigned int index, double subsample){
	//make sure we're in range
	if(mAudioData.size() <= channel)
		return 0.0;
	else if(mAudioData[channel].size() <= (index + 1))
		return 0.0;
	else
		return linear_interp(mAudioData[channel][index], mAudioData[channel][index + 1], subsample);
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
	inbuf = new float[READ_FRAME_SIZE * mSoundFile.channels()];
	chans = mSoundFile.channels();
   double num_frames = (double)mSoundFile.frames();
   unsigned int total_read = 0;

	while(!mAbort && (frames_read = mSoundFile.readf(inbuf, READ_FRAME_SIZE)) != 0){
		for(unsigned int i = 0; i < frames_read; i++){
			for(unsigned int j = 0; j < chans; j++){
				mAudioData[j].push_back(inbuf[i * chans + j]);
			}
		}

      //report progress
      if (progress_callback && num_frames != 0) {
         total_read += frames_read;
         progress_callback((double)(100 * total_read) / num_frames, user_data);
      }
	}
	delete [] inbuf;
   if (!mAbort) {
      mLoaded = true;
      if (progress_callback)
         progress_callback(100, user_data);
      return true;
   }
   return false;
}

void AudioBuffer::abort_load(){ mAbort = true; }
