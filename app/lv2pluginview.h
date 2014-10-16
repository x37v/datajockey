#ifndef LV2_PLUGIN_VIEW_H
#define LV2_PLUGIN_VIEW_H

#include "pluginview.h"

class Lv2Plugin;
class Lv2PluginView : public AudioPluginView {
  Q_OBJECT
  public:
    Lv2PluginView(Lv2Plugin * plugin, QWidget * parent = nullptr);
  private:
    SuilInstance * mInstance;
};

#endif
