#ifndef DJ_AUDIOANNOTION_HPP
#define DJ_AUDIOANNOTION_HPP

#include <stdexcept>
#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>
#include "beatbuffer.hpp"

namespace dj {
   namespace audio {
      class Annotation : public QObject {
         Q_OBJECT
         public:
            virtual ~Annotation();
            //TODO
            //void load_from_file(QString& file_path);
            
            //pass data like tags, artist, etc..
            void update_attributes(QHash<QString, QVariant>& attributes);
            void beat_buffer(BeatBufferPtr buffer);
            void write_file(const QString& file_path) throw(std::runtime_error);
            QString default_file_location(int work_id);
         private:
            QHash<QString, QVariant> mAttrs;
            BeatBufferPtr mBeatBuffer;
      };
   }
}

#endif
