#ifndef DATAJOCKEY_BEAT_EXTRACTOR_HPP
#define DATAJOCKEY_BEAT_EXTRACTOR_HPP

#include <QObject>
#include <vamp-hostsdk/PluginHostAdapter.h>
#include "audiobufferreference.hpp"
#include "beatbuffer.hpp"
#include <stdexcept>
#include <vector>

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
				size_t mBlockSize;
				size_t mStepSize;
				std::vector<float> mAnalBuffer;
				void allocate_plugin(int sample_rate) throw(std::runtime_error);
		};
	}
}

#endif
