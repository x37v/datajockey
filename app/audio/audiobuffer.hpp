#ifndef DATAJOCKEY_AUDIOBUFFER_HPP
#define DATAJOCKEY_AUDIOBUFFER_HPP

#include <stdexcept>
#include <vector>
#include "soundfile.hpp"
#include <QString>
#include <QSharedData>
#include <QExplicitlySharedDataPointer>

namespace djaudio {
  class AudioBuffer : public QSharedData {
    public:
      typedef std::vector<float > data_buffer_t; 
      typedef void (* progress_callback_t)(int percent, void * user_data);

      AudioBuffer(QString soundfileLocation) throw(std::runtime_error);
      virtual ~AudioBuffer();
      //returns true if completely loaded
      bool load(progress_callback_t progress_callback = NULL, void * user_data = NULL);
      void abort_load();

      //getters
      unsigned int sample_rate() const;
      unsigned int channels() const;
      unsigned int length() const;
      double seconds() const;

      bool loaded() const;
      //grab a sample
      float sample(unsigned int channel, unsigned int index) const;
      float sample(unsigned int channel, unsigned int index, double subsample) const;
      QString file_location() const;

      //fill a buffer, mixing to mono, starting at index start_index
      //expects the buffer to be resized to its desired fill size
      //zero pads the output buffer if you pass the end of the valid data
      void fill_mono(data_buffer_t& buffer, unsigned int start_index) const;

      bool valid() const;

      //get the data buffer
      const data_buffer_t& raw_buffer() const;
    private:
      SoundFile mSoundFile;
      data_buffer_t mAudioData;
      unsigned int mSampleRate;
      bool mLoaded;
      bool mAbort;
      unsigned int mNumChannels;
      float mMaxSample;
      bool mNormalize;
  };

  typedef QExplicitlySharedDataPointer<AudioBuffer> AudioBufferPtr;
}

#endif
