#include "audiobufferreference.hpp"
#include "audiobuffer.hpp"
#include "audiomodel.hpp"
#include <QMutexLocker>
#include <math.h>

using namespace DataJockey::Audio;

#define MAX(x,y) ((x) > (y) ? (x) : (y))
//#define MIN(x,y) MAX(y,x)
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#include <iostream>
using std::cout;
using std::endl;

//class objects
AudioBufferReference::manager_map_t AudioBufferReference::mBufferManager;
QMutex AudioBufferReference::mMutex;

//class methods
AudioBufferReference AudioBufferReference::get_reference(const QString& fileName){
   return AudioBufferReference(fileName);
}

void AudioBufferReference::decrement_count(const QString& fileName) {
   QMutexLocker lock(&mMutex);
   manager_map_t::iterator i = mBufferManager.find(fileName);
   //if its in our map then decrement it
   if (i != mBufferManager.end()) {
      i.value().first -= 1;
      //if we have reached zero reference count then destroy it
      if (i.value().first < 1) {
         delete i.value().second;
         mBufferManager.erase(i);
      }
   }
}

AudioBuffer * AudioBufferReference::get_and_increment_count(const QString& fileName){
   QMutexLocker lock(&mMutex);
   manager_map_t::iterator i = mBufferManager.find(fileName);

   if (i != mBufferManager.end()) {
      i.value().first += 1;
      return i.value().second;
   }
   return NULL;
}

#define DEFAULT_DIV 64

void AudioBufferReference::set_or_increment_count(const QString& fileName, AudioBuffer * buffer){
   QMutexLocker lock(&mMutex);
   manager_map_t::iterator i = mBufferManager.find(fileName);
   if (i != mBufferManager.end())
      i.value().first += 1;
   else
      mBufferManager[fileName] = ref_cnt_audio_buffer_t(1, buffer);

}


//instance methods
AudioBufferReference::AudioBufferReference() :
   mFileName(),
   mAudioBuffer(NULL)
{}

AudioBufferReference::AudioBufferReference(const QString& fileName) :
   mFileName(fileName),
   mAudioBuffer(NULL)
{
   mAudioBuffer = AudioBufferReference::get_and_increment_count(mFileName);
}

AudioBufferReference::AudioBufferReference(const AudioBufferReference& other){
   mFileName = other.mFileName;
   mAudioBuffer = AudioBufferReference::get_and_increment_count(mFileName);
}

AudioBufferReference& AudioBufferReference::operator=(const AudioBufferReference& other){
   if (&other != this) {
      release();
      mFileName = other.mFileName;
      mAudioBuffer = AudioBufferReference::get_and_increment_count(mFileName);
   }
   return *this;
}

AudioBufferReference::~AudioBufferReference() {
   release();
}

void AudioBufferReference::reset(const QString& newFileName) {
   release();
   mFileName = newFileName;
   mAudioBuffer = AudioBufferReference::get_and_increment_count(mFileName);
}

void AudioBufferReference::release() {
   AudioBufferReference::decrement_count(mFileName);
   mAudioBuffer = NULL;
   mFileName.clear();
}

AudioBuffer * AudioBufferReference::operator->() const {
   return mAudioBuffer;
}

bool AudioBufferReference::valid() {
   return mAudioBuffer != NULL;
}

