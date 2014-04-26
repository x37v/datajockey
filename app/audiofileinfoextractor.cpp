#include "audiofileinfoextractor.h"
#include "audiofiletag.h"
#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "beatextractor.h"
#include "config.hpp"
#include <QTemporaryFile>
#include <QDir>
#include <QtDebug>

namespace {

  unsigned int smoothing_iterations = 20; //XXX make it configurable


  //find the mid point between the previous and next values
  //find the difference between the data we have and that value, add 1/2 of that to the point
  unsigned int smoothed_point(unsigned int cur, unsigned int prev, unsigned int next) {
    double mid = static_cast<double>(next - prev) / 2.0 + static_cast<double>(prev);
    return static_cast<unsigned int>(cur + (mid - cur) / 2.0);
  }

  void smooth(std::deque<int>& data, unsigned int iterations) {
    if (data.empty() || data.size() < 4)
      return;
    for (unsigned int i = 0; i < iterations; i++) {
      //go forward, the backwards
      if (i % 2 == 0) {
        for (unsigned int j = 1; j < data.size() - 1; j++)
          data[j] = smoothed_point(data[j], data[j - 1], data[j + 1]);
      } else {
        for (unsigned int j = data.size() - 2; j > 0; j--)
          data[j] = smoothed_point(data[j], data[j - 1], data[j + 1]);
      }
    }
  }
}

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
  double max_seconds = dj::Configuration::instance()->import_max_seconds();
  try {
    //extract the tags
    audiofiletag::extract(audioFileName, tag_data);
    if (tag_data.find("name") == tag_data.end()) {
      emit(error(audioFileName, "tag data has no name entry"));
      return;
    }

    //extract the beats
    djaudio::AudioBufferPtr audio_buffer(new djaudio::AudioBuffer(audioFileName));
    djaudio::BeatBufferPtr beat_buffer(new djaudio::BeatBuffer);

    if (audio_buffer->channels() != 2) {
      QString msg = QString("only stereo files are currently supported, this file has %1 channel(s)").arg(audio_buffer->channels());
      emit(error(audioFileName, msg));
      return;
    }

    double seconds = audio_buffer->seconds();
    if (seconds == 0) {
      emit(error(audioFileName, QString("cannot find length of audio file")));
      return;
    } else if (seconds > max_seconds) {
      emit(error(audioFileName, QString("file exceeds maximum length allowed")));
      return;
    }

    //XXX use progress callbacks
    if (!audio_buffer->load()) {
      QString msg = QString("unknown error loading soundfile %1").arg(audioFileName);
      emit(error(audioFileName, msg));
      return;
    }
    mBeatExtractor->process(audio_buffer, beat_buffer);

    std::deque<int> dist = beat_buffer->distances();

    smooth(dist, smoothing_iterations);
    
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
    emit(fileCreated(audioFileName, temp_file.fileName(), tag_data));
  } catch (std::runtime_error& e) {
    emit(error(audioFileName, QString::fromStdString(e.what())));
  }
}

