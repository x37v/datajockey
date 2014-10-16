#include "pluginview.h"

#ifdef USE_LV2
#include "lv2pluginview.h"
#include "lv2plugin.h"
#endif

AudioPluginView * AudioPluginView::createView(AudioPlugin * plugin, QWidget * parent) {
#ifdef USE_LV2
  Lv2Plugin * lv2plugin = dynamic_cast<Lv2Plugin *>(plugin);
  if (lv2plugin) {
    Lv2PluginView * view = new Lv2PluginView(lv2plugin, parent);
    return view;
  }
#endif
  return nullptr;
}

AudioPluginView::AudioPluginView(QWidget* parent) : QWidget(parent) {
}

