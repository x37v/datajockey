#ifndef DJ_AUDIOANNOTION_HPP
#define DJ_AUDIOANNOTION_HPP

#include <stdexcept>
#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>
#include <deque>
#include <QExplicitlySharedDataPointer>

namespace djaudio {
  class BeatBuffer : public std::deque<int>, public QSharedData { };
  typedef QExplicitlySharedDataPointer<BeatBuffer> BeatBufferPtr;

  class Annotation : public QObject {
    Q_OBJECT
    public:
      bool loadFile(QString& file_path);

      //pass data like tags, artist, etc..
      void update_attributes(QHash<QString, QVariant>& attributes);
      void beat_buffer(BeatBufferPtr buffer);
      void write_file(const QString& file_path) throw(std::runtime_error);
      QString default_file_location(int work_id);
      BeatBufferPtr beatBuffer() const { return mBeatBuffer; }
    private:
      QHash<QString, QVariant> mAttrs;
      BeatBufferPtr mBeatBuffer;
  };
}

#endif
