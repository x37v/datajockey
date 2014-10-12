#include "plugin.h"
#include <lilv/lilv.h>
#include <stdexcept>
#include <map>
#include <vector>

class Lv2Plugin : public AudioPlugin {
  public:
    //constructor and destructor called in main thread
    Lv2Plugin(std::string uri, LilvWorld * world, const LilvPlugins * plugins) throw(std::runtime_error);
    virtual ~Lv2Plugin();
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length);

    //below called in audio thread
    virtual void compute(unsigned int nframes, float ** mixBuffer);
    virtual void stop();
    void control_value(uint32_t index, float v);
  private:
    LilvInstance * mLilvInstance;
    const LilvPlugin * mLilvPlugin;
    std::map<uint32_t, float *> mControls;
    std::vector<uint32_t> mAudioInputs;
    std::vector<uint32_t> mAudioOutputs;
};

