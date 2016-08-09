#include "plugin.h"

int AudioPlugin::cIndexCount = 0;

AudioPluginCollection::AudioPluginCollection() {
}

AudioPluginCollection::~AudioPluginCollection() {
}

void AudioPluginCollection::setup(unsigned int sample_rate, unsigned int max_buffer_length) {
  mEffects.each(
    [sample_rate, max_buffer_length](AudioPluginPtr plugin) {
      plugin->setup(sample_rate, max_buffer_length);
    });
}

void AudioPluginCollection::compute(unsigned int nframes, float ** mixBuffer) {
  mEffects.each(
    [nframes, &mixBuffer](AudioPluginPtr plugin) {
      plugin->compute(nframes, mixBuffer);
    });
}

void AudioPluginCollection::stop() {
  mEffects.each(
    [](AudioPluginPtr plugin) {
      plugin->stop();
    });
}

//do nothing
void AudioPluginCollection::control_value(uint32_t /*index*/, float /*v*/) { }

void AudioPluginCollection::append(AudioPluginNode * plugin) {
  mEffects.push_back(plugin);
}

void AudioPluginCollection::insert(unsigned int index, AudioPluginNode * plugin) {
}

AudioPlugin::AudioPlugin() {
  mIndex = cIndexCount++;
}

