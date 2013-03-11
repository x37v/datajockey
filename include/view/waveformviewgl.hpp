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

      Q_PROPERTY(QColor backgroudColor READ backgroud_color WRITE backgroud_color DESIGNABLE true)
      Q_PROPERTY(QColor waveformColor READ waveform_color WRITE waveform_color DESIGNABLE true)
      Q_PROPERTY(QColor cursorColor READ cursor_color WRITE cursor_color DESIGNABLE true)
      Q_PROPERTY(QColor centerLineColor READ centerLine_color WRITE centerLine_color DESIGNABLE true)
      Q_PROPERTY(QColor beatsColor READ beats_color WRITE beats_color DESIGNABLE true)

      public:
        WaveFormViewGL(QWidget * parent = NULL, bool vertical = false, bool full = false);
        QSize minimumSizeHint() const;
        QSize sizeHint() const;
        void setVertical(bool vert);

        void backgroud_color(QColor c) { backgroudColor = c; }
        QColor backgroud_color() const { return backgroudColor; }

        void waveform_color(QColor c) { waveformColor = c; }
        QColor waveform_color() const { return waveformColor; }

        void cursor_color(QColor c) { cursorColor = c; }
        QColor cursor_color() const { return cursorColor; }

        void centerLine_color(QColor c) { centerLineColor = c; }
        QColor centerLine_color() const { return centerLineColor; }

        void beats_color(QColor c) { beatsColor = c; }
        QColor beats_color() const { return beatsColor; }

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

        QColor backgroudColor;
        QColor waveformColor;
        QColor cursorColor;
        QColor centerLineColor;
        QColor beatsColor;

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
