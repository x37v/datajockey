#ifndef DATAJOCKEY_AUDIO_BUFFER_REFERENCE_H
#define DATAJOCKEY_AUDIO_BUFFER_REFERENCE_H

#include <QString>
#include <QMutex>
#include <QMap>
#include <QPair>

namespace DataJockey {
   namespace Audio {

      //forward declarations
      class AudioBuffer;

      class AudioBufferReference {
         //"class" methods
         public:
            //high level, does reference counting automatically
            static AudioBufferReference get_reference(const QString& fileName);

            //XXX low level/dangerous!
            static void decrement_count(const QString& fileName);
            static AudioBuffer * get_and_increment_count(const QString& fileName);
            static void set_or_increment_count(const QString& fileName, AudioBuffer * buffer);
         private:
            //filename => [refcount, buffer pointer]
            typedef QPair<int,  DataJockey::Audio::AudioBuffer *> ref_cnt_audio_buffer_t;
            typedef QMap<QString, ref_cnt_audio_buffer_t > manager_map_t;
            static manager_map_t mBufferManager;
            static QMutex mMutex;
         public:
            AudioBufferReference();
            AudioBufferReference(const QString& fileName);
            AudioBufferReference(const AudioBufferReference& other);
            AudioBufferReference& operator=(const AudioBufferReference& other);
            ~AudioBufferReference();
            void reset(const QString& newFileName);
            void release();
            bool valid();
            DataJockey::Audio::AudioBuffer * operator->() const;
         private:
            QString mFileName;
            AudioBuffer * mAudioBuffer;
      };
   }
}

#endif
