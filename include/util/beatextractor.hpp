#ifndef DATAJOCKEY_BEAT_EXTRACTOR_HPP
#define DATAJOCKEY_BEAT_EXTRACTOR_HPP

#include <QObject>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include "audiobufferreference.hpp"
#include "beatbuffer.hpp"
#include <stdexcept>

namespace DataJockey {
	namespace Util {
		class BeatExtractor : public QObject {
			Q_OBJECT
			public:
				BeatExtractor();
				virtual ~BeatExtractor();
				bool process(Audio::AudioBufferReference audio_buffer, Audio::BeatBuffer& beat_buffer) throw(std::runtime_error);
			signals:
				void progress(int percent);
			private:
				Vamp::Plugin * mPlugin;
				unsigned int mSampleRate;
				unsigned int mChannels;
				size_t mBlockSize;
				size_t mStepSize;
				void allocate_plugin(int sample_rate, unsigned int num_channels) throw(std::runtime_error);
		};
	}
}

#endif
