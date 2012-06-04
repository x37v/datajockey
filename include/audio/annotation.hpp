#ifndef DJ_AUDIOANNOTION_HPP
#define DJ_AUDIOANNOTION_HPP

#include <stdexcept>
#include <QObject>
#include <QString>
#include <QMap>
#include <QVariant>
#include "beatbuffer.hpp"

namespace dj {
   namespace audio {
      class Annotation : public QObject {
         Q_OBJECT
         public:
            //TODO
            //void load_from_file(QString& file_path);
            
            //pass data like tags, artist, etc..
            void update_attributes(QMap<QString, QVariant>& attributes);
            void beat_buffer(const BeatBuffer& buffer);
            void write_file(const QString& file_path) throw(std::runtime_error);
            QString default_file_location(int work_id);
         private:
            QMap<QString, QVariant> mAttrs;
            BeatBuffer mBeatBuffer;
      };
   }
}

#endif
