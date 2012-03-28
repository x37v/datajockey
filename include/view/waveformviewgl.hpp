#ifndef WAVEFORMVIEW_GL_HPP
#define WAVEFORMVIEW_GL_HPP

#include <QGLWidget>
#include <QtOpenGL>
#include <QColor>
#include <vector>

class WaveFormViewGL : public QGLWidget {
   Q_OBJECT
   public:
      WaveFormViewGL(QWidget * parent = NULL);
      QSize minimumSizeHint() const;
      QSize sizeHint() const;
   protected:
      void initializeGL();
      void paintGL();
      void resizeGL(int width, int height);
      //void mousePressEvent(QMouseEvent *event);
      //void mouseMoveEvent(QMouseEvent *event);
   private:
      int mHeight;
      int mWidth;
      bool mVertical;
      std::vector<GLfloat> mVerticies;
      QColor mColorBackgroud;
      QColor mColorWaveform;
      QColor mColorCursor;
      QColor mColorCenterLine;
};

#endif
