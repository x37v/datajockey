#include "audiofileinfoextractor.h"
#include "audiofiletag.h"
#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "beatextractor.h"
#include <QTemporaryFile>
#include <QDir>
#include <QtDebug>

AudioFileInfoExtractor::AudioFileInfoExtractor(QObject *parent) :
  QObject(parent)
{
  mBeatExtractor = new BeatExtractor;
}

AudioFileInfoExtractor::~AudioFileInfoExtractor() {
  delete mBeatExtractor;
}

void AudioFileInfoExtractor::processAudioFile(QString audioFileName) {
  QHash<QString, QVariant> tag_data;
  try {
    //extract the tags
    audiofiletag::extract(audioFileName, tag_data);

    //extract the beats
    djaudio::AudioBufferPtr audio_buffer(new djaudio::AudioBuffer(audioFileName));
    djaudio::BeatBufferPtr beat_buffer(new djaudio::BeatBuffer);

    //XXX use progress callbacks
    audio_buffer->load();
    mBeatExtractor->process(audio_buffer, beat_buffer);

    std::deque<int> dist = beat_buffer->distances();
    int median = djaudio::median(dist);
    float bpm = (60.0 * audio_buffer->sample_rate()) / median;
    tag_data["tempo_median"] = bpm;

    //create the annotation temp file
    djaudio::Annotation annotation;
    annotation.update_attributes(tag_data);
    annotation.beat_buffer(beat_buffer);

    QTemporaryFile temp_file(QDir::tempPath() + "/datajockey-XXXXXX.yaml");
    temp_file.setAutoRemove(false);
    annotation.write(temp_file);
    emit(fileCreated(audioFileName, temp_file.fileName()));
  } catch (std::runtime_error& e) {
    emit(error(audioFileName, QString::fromStdString(e.what())));
  }
}

