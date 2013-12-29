#ifndef WAVEFORMGL_H
#define WAVEFORMGL_H

#include <QObject>
#include <QVector>
#include <QtOpenGL>

#include "audiobuffer.hpp"

//for drawing a waveform within a gl view
//always draws horizontally, left to right
//draws from 0 to width in x, -1 to 1 in y
class WaveFormGL : public QObject
{
  public:
    WaveFormGL(QObject * parent = nullptr);
    int width() const { return mWidth; }
  public slots:
    void setAudioBuffer(djaudio::AudioBufferPtr buffer);
    void setPositionFrame(int frame);
    void setWidth(int pixels);
    void draw();
  private:
    struct glline_t{
      GLfloat x0; GLfloat y0;
      GLfloat x1; GLfloat y1;
    };
    djaudio::AudioBufferPtr mAudioBuffer;
    int mWidth = 400;
    int mFramesPerLine = 256;
    bool mZoomFull = true;
    QVector<glline_t> mLines;
    void updateLines();

    GLfloat lineHeight(int line_index) const;
};

#endif // WAVEFORMGL_H
