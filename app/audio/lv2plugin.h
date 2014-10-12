#include "plugin.h"
#include <lilv/lilv.h>
#include <QString>
#include <stdexcept>
#include <map>
#include <vector>

class Lv2Plugin : public AudioPlugin {
  public:
    //constructor and destructor called in main thread
    Lv2Plugin(QString uri, LilvWorld * world, const LilvPlugins * plugins) throw(std::runtime_error);
    virtual ~Lv2Plugin();
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length);

    QString port_name(uint32_t index);
    std::vector<uint32_t> control_input_ports() const;

    //below called in audio thread
    virtual void compute(unsigned int nframes, float ** mixBuffer);
    virtual void stop();
    void control_value(uint32_t index, float v);
  private:
    LilvInstance * mLilvInstance;
    const LilvPlugin * mLilvPlugin;
    uint32_t mNumPorts = 0;
    std::map<uint32_t, float *> mControlInputs;
    std::map<uint32_t, float *> mControlOutputs;
    std::vector<uint32_t> mAudioInputs;
    std::vector<uint32_t> mAudioOutputs;

    std::vector<float> mPortValueMin;
    std::vector<float> mPortValueMax;
    std::vector<float> mPortValueDefault;
    std::vector<QString> mPortNames;
};

