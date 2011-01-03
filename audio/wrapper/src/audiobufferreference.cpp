#include "audiobufferreference.hpp"
#include "audiobuffer.hpp"
#include "audiocontroller.hpp"
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
         delete i.value().second.first;
         if(i.value().second.second) {
            i.value().second.second->detach();
            delete i.value().second.second;
         }
         mBufferManager.erase(i);
      }
   }
}

AudioBuffer * AudioBufferReference::get_and_increment_count(const QString& fileName){
   QMutexLocker lock(&mMutex);
   manager_map_t::iterator i = mBufferManager.find(fileName);

   if (i != mBufferManager.end()) {
      i.value().first += 1;
      return i.value().second.first;
   }
   return NULL;
}

#define DIVISION 64

void AudioBufferReference::set_or_increment_count(const QString& fileName, AudioBuffer * buffer){
   QMutexLocker lock(&mMutex);
   manager_map_t::iterator i = mBufferManager.find(fileName);
   if (i != mBufferManager.end()) {
      i.value().first += 1;
   } else {
      QSharedMemory * shm = new QSharedMemory(fileName);
      if (shm->isAttached())
         shm->detach();

      const unsigned int buff_size = buffer->raw_buffer().size();
      const unsigned int num_elems = (buff_size / DIVISION);
      const unsigned int size = sizeof(float) * num_elems;

      if (shm->create(size, QSharedMemory::ReadWrite)) {
         cout << "we want " << size << " we've got " << shm->size() << endl;
         assert(size <= shm->size());

         cout << "successfully created shared memory! size:\t" << size << fileName.toStdString() << endl;
         shm->lock();
         float * data = (float *)shm->data();
         if (data) {
            for(unsigned int j = 0; j < num_elems; j++){
               float d = 0.0;
               unsigned int top = MIN(buff_size, (j + 1) * DIVISION);
               for(unsigned int k = j * DIVISION; k < top; k++) {
                  d = MAX(d, fabs(buffer->raw_buffer()[k]));
               }
               data[j] = d;
            }
         } else {
            cout << "could not get buffer pointer though" << endl;
         }
         mBufferManager[fileName] = QPair<int, buffer_shared_pair_t>(1, buffer_shared_pair_t(buffer, shm));
         shm->unlock();
      } else {
         cout << "failed to create shared memory! size:\t" << size << fileName.toStdString() << endl;
         cout << shm->errorString().toStdString() << endl;
         mBufferManager[fileName] = QPair<int, buffer_shared_pair_t>(1, buffer_shared_pair_t(buffer, NULL));
         delete shm;
      }
   }
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

AudioBuffer * AudioBufferReference::operator()() const {
   return mAudioBuffer;
}

bool AudioBufferReference::valid() {
   return mAudioBuffer != NULL;
}

