#include "lv2pluginview.h"
#include "lv2plugin.h"
#include <suil/suil.h>
#include <lilv/lilv.h>

namespace {
  SuilHost * suil_host = nullptr;
  const char * ui_type = "http://lv2plug.in/ns/extensions/ui#Qt5UI";

  void port_write(SuilController controller, uint32_t port_index, uint32_t buffer_size, uint32_t protocol, void const *buffer) {
  }

  uint32_t port_index(SuilController controller, const char *port_symbol) {
    return 0;
  }

  uint32_t port_subscribe(SuilController controller, uint32_t port_index, uint32_t protocol, const LV2_Feature *const *features) {
    return 0;
  }

  uint32_t port_unsubscribe(SuilController controller, uint32_t port_index, uint32_t protocol, const LV2_Feature *const *features) {
    return 0;
  }

  void port_touch(SuilController controller, uint32_t port_index, bool grabbed) {
  }
}

Lv2PluginView::Lv2PluginView(Lv2Plugin * plugin, QWidget * parent) :
  AudioPluginView(parent)
{
  if (!suil_host)
    suil_host = suil_host_new(port_write, port_index, port_subscribe, port_unsubscribe);

  mInstance = suil_instance_new(suil_host,
      NULL,
      ui_type,
      lilv_node_as_uri(lilv_plugin_get_uri(plugin->lilvplugin())),
      lilv_node_as_uri(lilv_ui_get_uri(jalv->ui)),
      lilv_node_as_uri(jalv->ui_type),
      lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_bundle_uri(jalv->ui))),
      lilv_uri_to_path(lilv_node_as_uri(lilv_ui_get_binary_uri(jalv->ui))),
      ui_features);
}

