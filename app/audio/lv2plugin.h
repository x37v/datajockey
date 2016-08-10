#include "plugin.h"
#include <lilv/lilv.h>
#include <QString>
#include <stdexcept>
#include <map>
#include <vector>

class Lv2Plugin : public AudioPlugin {
  public:
    //constructor and destructor called in main thread
    Lv2Plugin(QString uri) throw(std::runtime_error);
    virtual ~Lv2Plugin();
    virtual void setup(unsigned int sample_rate, unsigned int max_buffer_length);

    std::vector<uint32_t> control_input_ports() const;

    QString port_symbol(uint32_t index) const;
    float port_value_min(uint32_t index) const;
    float port_value_max(uint32_t index) const;
    float port_value_default(uint32_t index) const;
    uint32_t port_index(QString port_symbol) const throw(std::runtime_error);

    virtual int control_index(QString paramterName) const override;
    virtual void load_default_preset() override;
    //value expected to be between 0...1000 
    virtual double range_remap(int parameter_index, int value) override;

    void load_preset_from_file(QString file_path) throw(std::runtime_error);

    //below called in audio thread
    virtual void compute(unsigned int nframes, float ** mixBuffer);
    virtual void stop();
    virtual void control_value(uint32_t index, float v) override;
  private:
    QString mURI;
    LilvInstance * mLilvInstance;
    const LilvPlugin * mLilvPlugin;
    uint32_t mNumPorts = 0;
    std::map<uint32_t, float *> mControlInputs;
    std::map<uint32_t, float *> mControlOutputs;
    std::vector<uint32_t> mAudioInputs;
    std::vector<uint32_t> mAudioOutputs;

    std::vector<float> mComputeBuffer[2];

    std::vector<float> mPortValueMin;
    std::vector<float> mPortValueMax;
    std::vector<float> mPortValueDefault;
    std::vector<QString> mPortSymbols;

    std::map<uint32_t, int> mPortValueDBScale; //b.s. to work around calf's fucked up scaling
};

