#ifndef WAVEFORMVIEW_GL_HPP
#define WAVEFORMVIEW_GL_HPP

#include <QGLWidget>
#include <QtOpenGL>
#include <QMutex>
#include <QColor>
#include <vector>
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"

namespace dj {
  namespace view {
    class WaveFormViewGL : public QGLWidget {
      Q_OBJECT
      public:
        WaveFormViewGL(QWidget * parent = NULL, bool vertical = false, bool full = false);
        QSize minimumSizeHint() const;
        QSize sizeHint() const;
        void setVertical(bool vert);
      public slots:
        void clear_audio();
        void set_buffers(audio::AudioBufferPtr audio_buffer, audio::BeatBufferPtr beat_buffer);

        void clear_beats();

        void set_frame(int frame);
        void set_frames_per_line(int num_frames);

        void clear_markers();
        void add_marker(int id, int frame, QColor color);
        void remove_marker(int id);
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
        void frame_clicked(int frame);
      private:
        QMutex mMutex;

        int mHeight;
        int mWidth;
        int mCursorOffset;
        bool mVertical;
        bool mFullView;

        audio::AudioBufferPtr mAudioBuffer;
        std::vector<GLfloat> mVerticies;
        std::vector<float> mVertexColors;
        int mFirstLineIndex; //which is the first line
        bool mVerticiesValid;

        audio::BeatBufferPtr mBeatBuffer;
        std::vector<GLfloat> mBeatVerticies;
        bool mBeatVerticiesValid;
        float mSampleRate;

        struct marker_t {
          marker_t(int i, int f, QColor c) : id(i), frame(f), color(c) { }
          int id;
          int frame;
          QColor color;
        };
        QList<marker_t> mMarkers;
        std::vector<GLfloat> mMarkerVerticies;
        std::vector<float> mMarkerColors;

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
        void update_colors();
        void update_markers();
        GLfloat line_value(int line_index);
    };
  }
}

#endif
