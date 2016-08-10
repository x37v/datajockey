#include "lv2plugin.h"
#include <QFile>
#include <QTextStream>
#include "uridmap.h"
#include "symap.h"
#include <iostream>
#include "defines.hpp"
#include "config.hpp"

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

using std::cerr;
using std::endl;

namespace {
  LilvNode * lv2PortControl = nullptr;
  LilvNode * lv2PortAudio = nullptr;
  LilvNode * lv2PortInput = nullptr;
  LilvNode * lv2PortOutput = nullptr;

  LilvWorld * cLV2World = nullptr;
  const LilvPlugins * cLV2Plugins = nullptr;

  Symap * sym_map = nullptr;

  void setup_lilv() {
    if (cLV2World)
      return;

    //set up lv2
    cLV2World = lilv_world_new();
    if (cLV2World) {
      lilv_world_load_all(cLV2World);
      cLV2Plugins = lilv_world_get_all_plugins(cLV2World);
    }

    lv2PortAudio = lilv_new_uri(cLV2World, LILV_URI_AUDIO_PORT);
    lv2PortControl = lilv_new_uri(cLV2World, LILV_URI_CONTROL_PORT);
    lv2PortInput = lilv_new_uri(cLV2World, LILV_URI_INPUT_PORT);
    lv2PortOutput = lilv_new_uri(cLV2World, LILV_URI_OUTPUT_PORT);
    sym_map = symap_new();
    urid_sem_init();
  }

  //grabbed from ardour
  static void set_port_value(const char* port_symbol,
        void* user_data,
        const void* value,
        uint32_t /*size*/,
        uint32_t type)
    {
      Lv2Plugin* self = (Lv2Plugin*)user_data;
      if (type != 0 && type != urid_to_id(sym_map, LV2_ATOM__Float)) {
        return; // TODO: Support non-float ports
      }
      try {
        uint32_t port_index = self->port_index(port_symbol);
        float derefed = *(const float*)value;
        self->control_value(port_index, derefed);
      } catch (std::runtime_error& e) {
        cerr << "couldn't set port value: " + std::string(port_symbol) << endl;
      }
    }
}

Lv2Plugin::Lv2Plugin(QString uri) throw (std::runtime_error)
  : mURI(uri)
{
  setup_lilv();

  LilvNode * plugin_uri = lilv_new_uri(cLV2World, qPrintable(uri));
  mLilvPlugin = lilv_plugins_get_by_uri(cLV2Plugins, plugin_uri);
  if (!mLilvPlugin)
    throw std::runtime_error("could not load lv2 plugin with uri " + uri.toStdString());

  if (lilv_plugin_get_num_ports_of_class(mLilvPlugin, lv2PortAudio, lv2PortOutput, nullptr) != 2)
    throw std::runtime_error("not a stereo output plugin: " + uri.toStdString());
  if (lilv_plugin_get_num_ports_of_class(mLilvPlugin, lv2PortAudio, lv2PortInput, nullptr) != 2)
    throw std::runtime_error("not a stereo input plugin: " + uri.toStdString());

  mNumPorts = lilv_plugin_get_num_ports(mLilvPlugin);
  mPortValueMin.resize(mNumPorts, 0.0f);
  mPortValueMax.resize(mNumPorts, 0.0f);
  mPortValueDefault.resize(mNumPorts, 0.0f);
  lilv_plugin_get_port_ranges_float(mLilvPlugin, &mPortValueMin.front(), &mPortValueMax.front(), &mPortValueDefault.front());

  for (uint32_t i = 0; i < mNumPorts; i++) {
    LilvNode* n = lilv_port_get_name(mLilvPlugin, lilv_plugin_get_port_by_index(mLilvPlugin, i));
    mPortSymbols.push_back(QString(lilv_node_as_string(n)));
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

void Lv2Plugin::setup(unsigned int sample_rate, unsigned int max_buffer_length) {
  mLilvInstance = lilv_plugin_instantiate(mLilvPlugin, sample_rate, NULL);

  mComputeBuffer[0].resize(max_buffer_length, 0.0f);
  mComputeBuffer[1].resize(max_buffer_length, 0.0f);

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

std::vector<uint32_t> Lv2Plugin::control_input_ports() const {
  std::vector<uint32_t> indices;
  for (auto& kv: mControlInputs)
    indices.push_back(kv.first);
  return indices;
}

QString Lv2Plugin::port_symbol(uint32_t index) const {
  if (index >= mPortSymbols.size())
    return QString();
  return mPortSymbols[index];
}

float Lv2Plugin::port_value_min(uint32_t index) const {
  if (index >= mPortValueMin.size())
    return 0.0f;
  return mPortValueMin[index];
}

float Lv2Plugin::port_value_max(uint32_t index) const {
  if (index >= mPortValueMax.size())
    return 0.0f;
  return mPortValueMax[index];
}

float Lv2Plugin::port_value_default(uint32_t index) const {
  if (index >= mPortValueDefault.size())
    return 0.0f;
  return mPortValueDefault[index];
}

int Lv2Plugin::control_index(QString paramterName) const {
  try {
    return static_cast<int>(port_index(paramterName));
  } catch (...) {
  }
  return -1;
}

uint32_t Lv2Plugin::port_index(QString port_symbol) const throw(std::runtime_error) {
  LilvNode * snode = lilv_new_string(cLV2World, qPrintable(port_symbol));
  const LilvPort * port = lilv_plugin_get_port_by_symbol(mLilvPlugin, snode);
  lilv_node_free(snode);
  if (!port)
    throw std::runtime_error("cannot find port by symbol name: " + port_symbol.toStdString());
  return lilv_port_get_index(mLilvPlugin, port);
}

void Lv2Plugin::load_default_preset() {
  dj::Configuration * config = dj::Configuration::instance();
  QString preset = config->plugin_preset_file(mURI);
  if (preset.size())
    load_preset_from_file(preset);

  if (mURI == config->eq_plugin_uuid()) {
    for (int i = 0; i < 3; i++) {
      dj::eq_band_t band = static_cast<dj::eq_band_t>(i);
      uint32_t port = control_index(config->eq_port_symbol(band));
      mPortValueDBScale[port] = config->eq_band_db_scale(band);
    }
  }
}

double Lv2Plugin::range_remap(int parameter_index, int value) {
  float fvalue = dj::to_double(value);
  auto it = mPortValueDBScale.find(parameter_index);
  if (it != mPortValueDBScale.end()) {
    return dj::db2amp(it->second * fvalue);
  }
  if (parameter_index < 0 || parameter_index >= (int)mNumPorts)
    return 0;
  if (mPortValueMin[parameter_index] < 0) {
    if (fvalue < 0.0)
      return fvalue * -mPortValueMin[parameter_index];
    else
      return fvalue * mPortValueMax[parameter_index];
  } else {
    //otherwise map our -1..0..1 to min..default..max
    if (value >= 0.0)
      return mPortValueDefault[parameter_index] + fvalue * (mPortValueMax[parameter_index] - mPortValueDefault[parameter_index]);
    return mPortValueDefault[parameter_index] + fvalue * (mPortValueDefault[parameter_index] - mPortValueMin[parameter_index]);
  }

  return 0;
}

void Lv2Plugin::load_preset_from_file(QString file_path) throw(std::runtime_error) {
  //const LilvNode* uri = lilv_plugin_get_uri(mLilvPlugin);
  LV2_URID_Map map = { sym_map, map_urid };
  //LilvState* state = lilv_state_new_from_file(cLV2World, &map, uri, qPrintable(file_path));
  LilvState* state = lilv_state_new_from_file(cLV2World, &map, NULL, qPrintable(file_path));
  if (!state)
    throw std::runtime_error("couldn't load state from file: " + file_path.toStdString());
  lilv_state_restore(state, mLilvInstance, set_port_value, this, 0, NULL);
  //lilv_state_restore(state, mLilvInstance, NULL, NULL, 0, NULL);
  lilv_state_free(state);
}

void Lv2Plugin::compute(unsigned int nframes, float ** mixBuffer) {
  memset(&mComputeBuffer[0].front(), 0, sizeof(float) * nframes);
  memset(&mComputeBuffer[1].front(), 0, sizeof(float) * nframes);

  for (uint32_t i = 0; i < 2; i++) {
    lilv_instance_connect_port(mLilvInstance, mAudioInputs[i], mixBuffer[i]);
    lilv_instance_connect_port(mLilvInstance, mAudioOutputs[i], &mComputeBuffer[i].front());
  }
  lilv_instance_run(mLilvInstance, nframes);

  memcpy(mixBuffer[0], &mComputeBuffer[0].front(), sizeof(float) * nframes);
  memcpy(mixBuffer[1], &mComputeBuffer[1].front(), sizeof(float) * nframes);
}

void Lv2Plugin::stop() {
  lilv_instance_deactivate(mLilvInstance);
}

void Lv2Plugin::control_value(uint32_t index, float v) {
  auto it = mControlInputs.find(index);
  if (it == mControlInputs.end())
    return; //XXX error
  v = dj::clamp(v, (float)mPortValueMin[index], (float)mPortValueMax[index]);
  *(it->second) = v;
}

