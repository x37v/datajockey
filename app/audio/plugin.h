#ifndef AUDIO_PLUGIN_H
#define AUDIO_PLUGIN_H

#include "doublelinkedlist.h"
#include <QString>
#include <QSharedPointer>

class AudioPluginControlDescription {
  public:
    AudioPluginControlDescription() {}
    virtual ~AudioPluginControlDescription() {}

    virtual QString name() const;
    virtual QString value_string() const;
};

class AudioPlugin {
  public:
    //constructor and destructor called in main thread
    AudioPlugin();
    virtual ~AudioPlugin(){}
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length) = 0;

    //below called in audio thread
    virtual void compute(unsigned int nframes, float ** mixBuffer) = 0;
    virtual void stop(){}
    int index() const { return mIndex; }
  private:
    int mIndex = 0;
    static int cIndexCount;
};

typedef QSharedPointer<AudioPlugin> AudioPluginPtr;
typedef DoubleLinkedListNode<AudioPluginPtr> AudioPluginNode;

class AudioPluginCollection : public AudioPlugin {
  public:
    AudioPluginCollection();
    virtual ~AudioPluginCollection();
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length);

    virtual void compute(unsigned int nframes, float ** mixBuffer);
    virtual void stop();

    void insert(unsigned int index, AudioPluginNode * plugin);
    void append(AudioPluginNode * plugin); 
  private:
    DoubleLinkedList<AudioPluginPtr> mEffects;
};

#endif
