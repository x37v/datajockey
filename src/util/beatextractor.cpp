#include "beatextractor.hpp"
#include "audiobuffer.hpp"
#include <vamp-hostsdk/PluginInputDomainAdapter.h>
#include <vamp-hostsdk/PluginLoader.h>
#include <QMutex>
#include <QMutexLocker>

using namespace DataJockey::Util;

namespace {
	QMutex loaderMutex;
	const std::string pluginLibrary = "qm-vamp-plugins";
	const std::string pluginName =    "qm-barbeattracker";

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
}

BeatExtractor::BeatExtractor() : QObject(),
	mPlugin(NULL),
	mSampleRate(0),
	mChannels(0),
	mBlockSize(0),
	mStepSize(0)
{
}

BeatExtractor::~BeatExtractor() {
	if (mPlugin)
		delete mPlugin;
}

bool BeatExtractor::process(Audio::AudioBufferReference audio_buffer, Audio::BeatBuffer& beat_buffer)
	throw(std::runtime_error)
{
	//make sure we have a valid plugin and that its rate is correct
	if (mPlugin == NULL) {
		allocate_plugin(audio_buffer->sample_rate(), audio_buffer->channels());
	} else if (mSampleRate != audio_buffer->sample_rate() || mChannels != audio_buffer->channels()) {
		delete mPlugin;
		allocate_plugin(audio_buffer->sample_rate(), audio_buffer->channels());
	} else {
		mPlugin->reset();
	}

	//XXX DO IT
	
	return true;
}

void BeatExtractor::allocate_plugin(int sample_rate, unsigned int num_channels)
	throw(std::runtime_error)
{
	mSampleRate = sample_rate;
	mChannels = num_channels;
	mPlugin = load_plugin(mSampleRate);
	mBlockSize = mPlugin->getPreferredBlockSize();
	mStepSize = mPlugin->getPreferredStepSize();
	if (mBlockSize == 0)
		mBlockSize = 1024;
	if (mStepSize == 0 || mStepSize > mBlockSize)
		mStepSize = mBlockSize;
	if (!mPlugin->initialise(mChannels, mStepSize, mBlockSize)) {
		delete mPlugin;
		mPlugin = NULL;
		throw std::runtime_error("BeatExtractor::allocate_plugin could not initialise plugin");
	}
}
