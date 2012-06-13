#ifndef WAVEFORMVIEW_GL_HPP
#define WAVEFORMVIEW_GL_HPP

#include <QGLWidget>
#include <QtOpenGL>
#include <QMutex>
#include <QColor>
#include <vector>
#include "audiobufferreference.hpp"
#include "beatbuffer.hpp"

namespace dj {
   namespace view {
      class WaveFormViewGL : public QGLWidget {
         Q_OBJECT
         public:
            WaveFormViewGL(QWidget * parent = NULL, bool vertical = false);
            QSize minimumSizeHint() const;
            QSize sizeHint() const;
            void setVertical(bool vert);
         public slots:
            void clear_audio();
            void set_audio_file(QString file_name);

            void clear_beats();
            void set_beat_buffer(audio::BeatBufferPtr buffer);

            void set_frame(int frame);
            void set_frames_per_line(int num_frames);
         protected:
            void initializeGL();
            void paintGL();
            void resizeGL(int width, int height);
            //virtual void wheelEvent(QWheelEvent * event);
            virtual void mouseMoveEvent(QMouseEvent * event);
            virtual void mousePressEvent(QMouseEvent * event);
            virtual void mouseReleaseEvent(QMouseEvent * event);
         signals:
            void seek_relative(int);
            void mouse_down(bool);
         private:
            QMutex mMutex;

            int mHeight;
            int mWidth;
            int mCursorOffset;
            bool mVertical;

            audio::AudioBufferReference mAudioBuffer;
            std::vector<GLfloat> mVerticies;
            int mFirstLineIndex; //which is the first line
            bool mVerticiesValid;

            audio::BeatBufferPtr mBeatBuffer;
            std::vector<GLfloat> mBeatVerticies;
            bool mBeatVerticiesValid;
            float mSampleRate;

            int mFramesPerLine;
            int mFrame;

            QColor mColorBackgroud;
            QColor mColorWaveform;
            QColor mColorCursor;
            QColor mColorCenterLine;
            QColor mColorBeats;

            int mLastMousePos;

            void update_waveform();
            void update_beats();
            GLfloat line_value(int line_index);
      };
   }
}

#endif
