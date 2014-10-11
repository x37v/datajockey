#ifndef AUDIO_PLUGIN_H
#define AUDIO_PLUGIN_H

#include "doublelinkedlist.h"

class AudioPlugin {
  public:
    //constructor and destructor called in main thread
    virtual ~AudioPlugin(){}
    //below called in audio thread
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length) = 0;
    virtual void compute(unsigned int nframes, float ** mixBuffer) = 0;
    virtual void stop(){}
};

class AudioPluginCollection : public AudioPlugin {
  public:
    AudioPluginCollection();
    virtual ~AudioPluginCollection();
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length);
    virtual void compute(unsigned int nframes, float ** mixBuffer);
    virtual void stop();
  private:
    DoubleLinkedList<AudioPlugin *> mEffects;
};

#endif
