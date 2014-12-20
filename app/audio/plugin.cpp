#include "plugin.h"

unsigned int AudioPlugin::cIndexCount = 0;

AudioPluginCollection::AudioPluginCollection() {
}

AudioPluginCollection::~AudioPluginCollection() {
}

void AudioPluginCollection::setup(unsigned int sample_rate, unsigned int max_buffer_length) {
  mEffects.each(
    [sample_rate, max_buffer_length](AudioPlugin * plugin) {
      plugin->setup(sample_rate, max_buffer_length);
    });
}

void AudioPluginCollection::compute(unsigned int nframes, float ** mixBuffer) {
  mEffects.each(
    [nframes, &mixBuffer](AudioPlugin * plugin) {
      plugin->compute(nframes, mixBuffer);
    });
}

void AudioPluginCollection::stop() {
  mEffects.each(
    [](AudioPlugin * plugin) {
      plugin->stop();
    });
}

void AudioPluginCollection::append(AudioPluginNode * plugin) {
  mEffects.push_back(plugin);
}

AudioPlugin::AudioPlugin() {
  mIndex = cIndexCount++;
}

