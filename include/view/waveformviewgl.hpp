#ifndef WAVEFORMVIEW_GL_HPP
#define WAVEFORMVIEW_GL_HPP

#include <QGLWidget>
#include <QtOpenGL>
#include <QMutex>
#include <QColor>
#include <vector>
#include "audiobufferreference.hpp"

namespace DataJockey {
   namespace View {
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
            void set_frame(int frame);
            void set_frames_per_line(int num_frames);
         protected:
            void initializeGL();
            void paintGL();
            void resizeGL(int width, int height);
            //void mousePressEvent(QMouseEvent *event);
            //void mouseMoveEvent(QMouseEvent *event);
         private:
            int mHeight;
            int mWidth;
            int mCursorOffset;
            bool mVertical;
            std::vector<GLfloat> mVerticies;
            int mFirstLineIndex; //which is the first line
            bool mVerticiesValid;
            int mFramesPerLine;
            int mFrame;
            QColor mColorBackgroud;
            QColor mColorWaveform;
            QColor mColorCursor;
            QColor mColorCenterLine;
            Audio::AudioBufferReference mAudioBuffer;
            QMutex mMutex;
            void update_waveform();
            GLfloat line_value(int line_index);
      };
   }
}

#endif
