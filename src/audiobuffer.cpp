#include "audiobuffer.hpp"
#include "soundfile.hpp"

#define READ_FRAME_SIZE 2048

template<typename Type>
Type linear_interp(Type v0, Type v1, double dist){
	return v0 + (v1 - v0) * dist;
}

using namespace DataJockey;

AudioBuffer::AudioBuffer(std::string soundfileLocation, AudioBuffer::progress_callback_t progress_callback) 
	throw(std::runtime_error){
      mProgressCallback = progress_callback;
		load(soundfileLocation);
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

void AudioBuffer::load(std::string soundfileLocation) 
	throw(std::runtime_error){
	float * inbuf = NULL;
	unsigned int frames_read;
	unsigned int chans;
	SoundFile sndFile(soundfileLocation.c_str());
	mAudioData.clear();

	//check to make sure soundfile exists
	if(!sndFile){
		std::string str("cannot open soundfile: ");
		str.append(soundfileLocation);
		throw std::runtime_error(str);
	}
	//read in the audio data
	inbuf = new float[READ_FRAME_SIZE * sndFile.channels()];
	chans = sndFile.channels();
	mSampleRate = sndFile.samplerate();
	//we must resize the audio buffer because we want to make sure that
	//we can do mAudioData[i].push_back
	mAudioData.resize(chans);
   double num_frames = (double)sndFile.frames();
   unsigned int total_read = 0;

	while((frames_read = sndFile.readf(inbuf, READ_FRAME_SIZE)) != 0){
		for(unsigned int i = 0; i < frames_read; i++){
			for(unsigned int j = 0; j < chans; j++){
				mAudioData[j].push_back(inbuf[i * chans + j]);
			}
		}

      //report progress
      if (mProgressCallback && num_frames != 0) {
         total_read += frames_read;
         mProgressCallback((double)(100 * total_read) / num_frames, NULL);
      }
	}
	delete [] inbuf;
   if (mProgressCallback) {
      mProgressCallback(100, NULL);
   }
}
