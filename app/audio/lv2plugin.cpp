#include "lv2plugin.h"

namespace {
  LilvNode * lv2PortControl = nullptr;
  LilvNode * lv2PortAudio = nullptr;
  LilvNode * lv2PortInput = nullptr;
  LilvNode * lv2PortOutput = nullptr;

  void setup_lilv(LilvWorld * world) {
    if (lv2PortControl)
      return;
    lv2PortAudio = lilv_new_uri(world, LILV_URI_AUDIO_PORT);
    lv2PortControl = lilv_new_uri(world, LILV_URI_CONTROL_PORT);
    lv2PortInput = lilv_new_uri(world, LILV_URI_INPUT_PORT);
    lv2PortOutput = lilv_new_uri(world, LILV_URI_OUTPUT_PORT);
  }
}

Lv2Plugin::Lv2Plugin(std::string uri, LilvWorld * world, const LilvPlugins * plugins) throw (std::runtime_error) {
  setup_lilv(world);

  LilvNode * plugin_uri = lilv_new_uri(world, uri.c_str());
  mLilvPlugin = lilv_plugins_get_by_uri(plugins, plugin_uri);
  if (!mLilvPlugin)
    throw std::runtime_error("could not load lv2 plugin with uri " + uri);

  if (lilv_plugin_get_num_ports_of_class(mLilvPlugin, lv2PortAudio, lv2PortOutput, nullptr) != 2)
    throw std::runtime_error("not a stereo output plugin: " + uri);
  if (lilv_plugin_get_num_ports_of_class(mLilvPlugin, lv2PortAudio, lv2PortInput, nullptr) != 2)
    throw std::runtime_error("not a stereo input plugin: " + uri);

  mNumPorts = lilv_plugin_get_num_ports(mLilvPlugin);
  mPortValueMin.resize(mNumPorts, 0.0f);
  mPortValueMax.resize(mNumPorts, 0.0f);
  mPortValueDefault.resize(mNumPorts, 0.0f);
  lilv_plugin_get_port_ranges_float(mLilvPlugin, &mPortValueMin.front(), &mPortValueMax.front(), &mPortValueDefault.front());

  for (uint32_t i = 0; i < mNumPorts; i++) {
    LilvNode* n = lilv_port_get_name(mLilvPlugin, lilv_plugin_get_port_by_index(mLilvPlugin, i));
    mPortNames.push_back(std::string(lilv_node_as_string(n)));
    lilv_node_free(n);
  }
}

Lv2Plugin::~Lv2Plugin() {
  lilv_instance_free(mLilvInstance);
  for (auto& kv: mControlInputs)
    delete kv.second;
  for (auto& kv: mControlOutputs)
    delete kv.second;
}

void Lv2Plugin::setup(unsigned int sample_rate, unsigned int /* max_buffer_length */) {
  mLilvInstance = lilv_plugin_instantiate(mLilvPlugin, sample_rate, NULL);

  for (uint32_t i = 0; i < mNumPorts; i++) {
    const LilvPort * port = lilv_plugin_get_port_by_index(mLilvPlugin, i);
    if (lilv_port_is_a(mLilvPlugin, port, lv2PortControl)) {
      //XXX not sure if a map copies data around so, using a pointer
      float * v = new float;
      *v = mPortValueDefault[i]; //set it to the default value
      if (lilv_port_is_a(mLilvPlugin, port, lv2PortInput)) {
        mControlInputs[i] = v;
      } else if (lilv_port_is_a(mLilvPlugin, port, lv2PortOutput)) {
        mControlOutputs[i] = v;
      } else {
        //XXX??
        delete v;
        continue;
      }
      lilv_instance_connect_port(mLilvInstance, i, v);
    } else if (lilv_port_is_a(mLilvPlugin, port, lv2PortAudio)) {
      if (lilv_port_is_a(mLilvPlugin, port, lv2PortInput)) {
        mAudioInputs.push_back(i);
      } else if (lilv_port_is_a(mLilvPlugin, port, lv2PortOutput)) {
        mAudioOutputs.push_back(i);
      } else {
        //XXX?
      }
    } else {
      //XXX?
    }
  }
  lilv_instance_activate(mLilvInstance);
}

std::string Lv2Plugin::port_name(uint32_t index) {
  if (index >= mPortNames.size())
    return std::string();
  return mPortNames[index];
}

std::vector<uint32_t> Lv2Plugin::control_input_ports() const {
  std::vector<uint32_t> indices;
  for (auto& kv: mControlInputs)
    indices.push_back(kv.first);
  return indices;
}

void Lv2Plugin::compute(unsigned int nframes, float ** mixBuffer) {
  for (uint32_t i = 0; i < 2; i++) {
    lilv_instance_connect_port(mLilvInstance, mAudioInputs[i], mixBuffer[i]);
    lilv_instance_connect_port(mLilvInstance, mAudioOutputs[i], mixBuffer[i]);
  }
  lilv_instance_run(mLilvInstance, nframes);
}

void Lv2Plugin::stop() {
  lilv_instance_deactivate(mLilvInstance);
}

void Lv2Plugin::control_value(uint32_t index, float v) {
  auto it = mControlInputs.find(index);
  if (it == mControlInputs.end())
    return; //XXX error
  *(it->second) = v;
}

