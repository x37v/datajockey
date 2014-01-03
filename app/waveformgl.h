#ifndef WAVEFORMGL_H
#define WAVEFORMGL_H

#include <QObject>
#include <QVector>
#include <QtOpenGL>
#include <QColor>

#include "audiobuffer.hpp"

//for drawing a waveform within a gl view
//always draws horizontally, left to right
//draws from 0 to width in x, -1 to 1 in y
class WaveFormGL : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QColor cursorColor READ cursorColorGet WRITE cursorColorSet DESIGNABLE true)

  public:
    WaveFormGL(QObject * parent = nullptr);
    int width() const { return mWidth; }

    QColor cursorColorGet() const { return cursorColor; }
    void cursorColorSet(QColor v) { cursorColor = v; }

    void setWidth(int pixels);
    void historyWidth(int pixels);
    void zoomFull(bool v) { mZoomFull = v; }
    bool zoomFull() const { return mZoomFull; }
    int framesPerLine() const { return mFramesPerLine; }

  public slots:
    void setAudioBuffer(djaudio::AudioBufferPtr buffer);
    void setPositionFrame(int frame);
    void framesPerLine(int v);
    void draw();

  private:
    struct glline_t{
      GLfloat x0; GLfloat y0;
      GLfloat x1; GLfloat y1;
    };
    struct gl2triangles_t{
      GLfloat x0; GLfloat y0;
      GLfloat x1; GLfloat y1;
      GLfloat x2; GLfloat y2;
      GLfloat x3; GLfloat y3;
      GLfloat x4; GLfloat y4;
      GLfloat x5; GLfloat y5;
      //rect
      void rect(GLfloat rectx0, GLfloat recty0, GLfloat rectx1, GLfloat recty1) {
        x0 = rectx0;
        y0 = recty0;

        x1 = x4 = rectx0;
        y1 = y4 = recty1;

        x5 = rectx1;
        y5 = recty1;

        x2 = x3 = rectx1;
        y2 = y3 = recty0;
      }
    };
    djaudio::AudioBufferPtr mAudioBuffer;
    int mWidth = 400;
    int mHistoryWidth = 50;
    int mFramesPerLine = 512;
    int mFramePosition = 0;
    bool mZoomFull = true;
    QVector<gl2triangles_t> mLines;
    QColor cursorColor = Qt::white;
    void updateLines();

    GLfloat lineHeight(int line_index) const;
};

#endif // WAVEFORMGL_H
