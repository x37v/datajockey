#include "audio/pluginmanager.h"
#include <QMutex>
#include "defines.hpp"
#ifdef USE_LV2
#include "lv2plugin.h"
#endif

namespace {
  AudioPluginManager * cInstance = nullptr;
  QMutex cInstanceMutex;
}

AudioPluginManager * AudioPluginManager::instance() {
  QMutexLocker lock(&cInstanceMutex);
  if (!cInstance)
    cInstance = new AudioPluginManager();
  return cInstance;
}

AudioPluginPtr AudioPluginManager::instance(int instanceid) {
  auto it = mPlugins.find(instanceid);
  if (it == mPlugins.end())
    return AudioPluginPtr();
  return *it;
}

AudioPluginPtr AudioPluginManager::create(QString uniqueId) {
  AudioPluginPtr plugin;
#ifdef USE_LV2
  plugin = AudioPluginPtr(new Lv2Plugin(uniqueId));
#endif
  if (plugin)
    mPlugins[plugin->index()] = plugin;
  return plugin;
}

void AudioPluginManager::destroy(AudioPluginPtr plugin) {
  if (!plugin)
    return;
  mPlugins.remove(plugin->index());
}

double AudioPluginManager::range_remap(int plugin_index, int parameter_index, int value) {
  auto it = mPlugins.find(plugin_index);
  if (it == mPlugins.end())
    return dj::to_double(value);
  return (*it)->range_remap(parameter_index, value);
}

AudioPluginManager::AudioPluginManager() : QObject() { }

void AudioPluginManager::playerSetValueInt(int /*player*/, QString /*name*/, int /*value*/) {
}

