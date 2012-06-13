#include "beatextractor.hpp"
#include "audiobuffer.hpp"
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>
#include <QMutex>
#include <QMutexLocker>

using namespace dj::util;

namespace {
	QMutex loaderMutex;
	const std::string pluginLibrary = "qm-vamp-plugins";
	const std::string pluginName =    "qm-barbeattracker";
	const unsigned int beat_output_index = 0; //which output from the plugin actually gives the beat locations

	Vamp::HostExt::PluginLoader *vampLoader = NULL;
	Vamp::HostExt::PluginLoader::PluginKey pluginKey;
	int vampPluginAdapterFlags = Vamp::HostExt::PluginLoader::ADAPT_ALL_SAFE;

	Vamp::Plugin * load_plugin(int sample_rate) {
		QMutexLocker lock(&loaderMutex);
		if (vampLoader == NULL) {
			vampLoader = Vamp::HostExt::PluginLoader::getInstance();
			pluginKey = vampLoader->composePluginKey(pluginLibrary, pluginName);
		}
		return vampLoader->loadPlugin(pluginKey, sample_rate, vampPluginAdapterFlags);
	}

	inline double vamp_realtime_to_seconds(const Vamp::RealTime& rt) {
		return (double)rt.sec + (double)rt.usec() / 1000000.0;
	}
}

BeatExtractor::BeatExtractor() : QObject(),
	mPlugin(NULL),
	mSampleRate(0),
	mBlockSize(0),
	mStepSize(0)
{
}

BeatExtractor::~BeatExtractor() {
	if (mPlugin)
		delete mPlugin;
}

bool BeatExtractor::process(const audio::AudioBuffer& audio_buffer, audio::BeatBufferPtr beat_buffer)
	throw(std::runtime_error)
{
	//make sure we have a valid plugin and that its rate is correct
	if (mPlugin == NULL) {
		allocate_plugin(audio_buffer.sample_rate());
	} else if (mSampleRate != audio_buffer.sample_rate()) {
		delete mPlugin;
		allocate_plugin(audio_buffer.sample_rate());
	} else {
		mPlugin->reset();
	}

	beat_buffer->clear();

	Vamp::Plugin::FeatureSet features;
	const unsigned int audio_frames = audio_buffer.length();
	const unsigned int last_block = (audio_frames - mBlockSize);

	unsigned int progress_last = 0;
	unsigned int progress_report = audio_buffer.length() / 100;
	if (progress_report == 0)
		progress_report = 1;

	for (unsigned int i = 0; i <= last_block; i += mStepSize) {
		audio_buffer.fill_mono(mAnalBuffer, i);
		const float * bufptr = &mAnalBuffer.front();
		features = mPlugin->process(&bufptr, Vamp::RealTime::frame2RealTime(i, mSampleRate));
		for (unsigned int f = 0; f < features[beat_output_index].size(); f++)
			beat_buffer->insert_beat(vamp_realtime_to_seconds(features[beat_output_index][f].timestamp));

		//TODO make this based on block size and a modulus for less math?
		if ((i - progress_last) >= progress_report) {
			emit(progress(i * 100 / audio_frames));
			progress_last = i;
		}
	}

	features = mPlugin->getRemainingFeatures();
	for (unsigned int f = 0; f < features[beat_output_index].size(); f++)
		beat_buffer->insert_beat(vamp_realtime_to_seconds(features[beat_output_index][f].timestamp));

	emit(progress(100));
	return true;
}

void BeatExtractor::allocate_plugin(int sample_rate)
	throw(std::runtime_error)
{
	mSampleRate = sample_rate;
	mPlugin = load_plugin(mSampleRate);
	mBlockSize = mPlugin->getPreferredBlockSize();
	mStepSize = mPlugin->getPreferredStepSize();
	if (mBlockSize == 0)
		mBlockSize = 1024;
	if (mStepSize == 0 || mStepSize > mBlockSize)
		mStepSize = mBlockSize;
	if (!mPlugin->initialise(1, mStepSize, mBlockSize)) {
		delete mPlugin;
		mPlugin = NULL;
		throw std::runtime_error("BeatExtractor::allocate_plugin could not initialise plugin");
	}

	mAnalBuffer.resize(mBlockSize);
}
