#ifndef WAVEFORMGL_H
#define WAVEFORMGL_H

#include <QObject>
#include <QVector>
#include <QtOpenGL>
#include <QColor>
#include <QThread>

#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "defines.hpp"

class WavedataCalculator;

struct glcolor_t {
  GLfloat red;
  GLfloat green;
  GLfloat blue;

  glcolor_t(QColor color = Qt::yellow) { set(color); }
  glcolor_t(const glcolor_t& other) {
    red = other.red;
    green = other.green;
    blue = other.blue;
  }
  void set(QColor color) {
    red = color.redF();
    green = color.greenF();
    blue = color.blueF();
  }
};

struct gl2triangles_t{
  GLfloat x0; GLfloat y0; GLfloat z0;
  GLfloat x1; GLfloat y1; GLfloat z1;
  GLfloat x2; GLfloat y2; GLfloat z2;
  GLfloat x3; GLfloat y3; GLfloat z3;
  GLfloat x4; GLfloat y4; GLfloat z4;
  GLfloat x5; GLfloat y5; GLfloat z5;
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

    z0 = z1 = z2 = z3 = z4 = z5 = 0;
  }

  void set(glcolor_t color) {
    x0 = x1 = x2 = x3 = x4 = x5 = color.red;
    y0 = y1 = y2 = y3 = y4 = y5 = color.green;
    z0 = z1 = z2 = z3 = z4 = z5 = color.blue;
  }

  void set(QColor color) {
    glcolor_t c(color);
    set(c);
  }
};

Q_DECLARE_METATYPE(gl2triangles_t)

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

    int frameAtX(GLfloat x) const;

    struct glline_t{
      GLfloat x0; GLfloat y0;
      GLfloat x1; GLfloat y1;
    };

    struct marker_t {
      int frame_start;
      int frame_end;
      QString label;
    };

  public slots:
    void setAudioBuffer(djaudio::AudioBufferPtr buffer);
    void setBeatBuffer(djaudio::BeatBufferPtr buffer);
    void setPositionFrame(int frame);
    void framesPerLine(int v);
    void draw();
    void drawText(QPainter * painter, float width_scale);

    void updateMarker(dj::loop_and_jump_type_t type, int entry, int start, int end);
    void clearMarker(int entry);
    void clearAllMarkers();
  signals:
    void waveformLinesRequested(djaudio::AudioBufferPtr buffer, int startLine, int endLine, int framesPerLine);
    void colorsRequested(djaudio::BeatBufferPtr buffer, int lines, int framesPerLine);
  protected slots:
    void setBeatLines(QVector<glline_t> lines);
    void setWaveformLine(int lineIndex, gl2triangles_t value);
    void setColor(int lineIndex, QColor color);

  private:
    djaudio::AudioBufferPtr mAudioBuffer;
    djaudio::BeatBufferPtr mBeatBuffer;
    int mWidth = 800;
    int mHistoryWidth = 100;
    int mFramesPerLine = 512;
    int mFramePosition = 0;
    int mXStartLast;

    QThread * mCalculateThread;
    WavedataCalculator * mWaveformCalculator;

    bool mZoomFull = true;
    QVector<gl2triangles_t> mWaveformLines;
    QVector<gl2triangles_t> mWaveformColors;
    QVector<glline_t> mBeatLines;
    QHash<int, marker_t> mMarkers;
    QColor cursorColor = Qt::white;
    QColor mWaveformColor = Qt::darkRed;
    QColor mBeatColor = Qt::yellow;
    QColor mMarkerColorJump = Qt::cyan;
    QColor mMarkerColorLoop = Qt::white;
    void updateLines();

    GLfloat lineHeight(int line_index) const;
};

class WavedataCalculator : public QObject {
  Q_OBJECT
  public:
    WavedataCalculator(QObject * parent = nullptr);
    virtual ~WavedataCalculator() {}
  public slots:
    void compute(djaudio::AudioBufferPtr buffer, int startLine, int endLine, int framesPerLine);
    void computeColors(djaudio::BeatBufferPtr beats, int lines, int framesPerLine);
  signals:
    void lineChanged(int lineIndex, gl2triangles_t line);
    void colorChanged(int lineIndex, QColor color);

  private:
    QColor mWaveformColor = Qt::darkRed;
    QColor mWaveformColorOffSlow = Qt::yellow;
    QColor mWaveformColorOffFast = Qt::magenta;
};

#endif // WAVEFORMGL_H
