#ifndef MIXERPANELWAVEFORMSVIEW_H
#define MIXERPANELWAVEFORMSVIEW_H

#include <QWidget>
#include <QGLWidget>
#include <QtOpenGL>
#include <QColor>

#include "audiobuffer.hpp"
#include "annotation.hpp"
#include "defines.hpp"

class WaveFormGL;

class MixerPanelWaveformsView : public QGLWidget
{
  Q_OBJECT

  Q_PROPERTY(QColor backgroudColor READ backgroudColorGet WRITE backgroudColorSet DESIGNABLE true)
  Q_PROPERTY(QColor waveformColor READ waveformColorGet WRITE waveformColorSet DESIGNABLE true)
  Q_PROPERTY(QColor cursorColor READ cursorColorGet WRITE cursorColorSet DESIGNABLE true)
  Q_PROPERTY(QColor centerLineColor READ centerLineColorGet WRITE centerLineColorSet DESIGNABLE true)
  Q_PROPERTY(QColor dividerLineColor READ dividerLineColorGet WRITE dividerLineColorSet DESIGNABLE true)
  Q_PROPERTY(QColor beatsColor READ beatsColorGet WRITE beatsColorSet DESIGNABLE true)

  Q_PROPERTY(int fullWaveformWidth READ fullWaveformWidthGet WRITE fullWaveformWidthSet DESIGNABLE true)
  Q_PROPERTY(int waveformPadding READ waveformPaddingGet WRITE waveformPaddingSet DESIGNABLE true)

  public:
    explicit MixerPanelWaveformsView(QWidget *parent = 0);

    QColor backgroudColorGet() const { return backgroudColor; }
    void backgroudColorSet(QColor v) { backgroudColor = v; }

    QColor waveformColorGet() const { return waveformColor; }
    void waveformColorSet(QColor v) { waveformColor = v; }

    QColor cursorColorGet() const { return cursorColor; }
    void cursorColorSet(QColor v) { cursorColor = v; }

    QColor centerLineColorGet() const { return centerLineColor; }
    void centerLineColorSet(QColor v) { centerLineColor = v; }

    QColor dividerLineColorGet() const { return dividerLineColor; }
    void dividerLineColorSet(QColor v) { dividerLineColor = v; }

    QColor beatsColorGet() const { return beatsColor; }
    void beatsColorSet(QColor v) { beatsColor = v; }

    int fullWaveformWidthGet() const { return fullWaveformWidth; }
    void fullWaveformWidthSet(int v) { fullWaveformWidth = v; }

    int waveformPaddingGet() const { return waveformPadding; }
    void waveformPaddingSet(int v) { waveformPadding = v; }

  public slots:
    void playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
    void playerSetValueInt(int player, QString name, int v);

    void updateMarker(int player, dj::loop_and_jump_type_t type, int entry, int frame_start, int frame_end);
    void clearMarker(int player, int entry);
    void clearAllMarkers(int player);
  signals:
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);
  private:
    QList<WaveFormGL *> mWaveforms;
    QList<QPair<GLfloat, GLfloat> > mOffsetAndScale;
    int mNumPlayers = 2;
    int mSeekingWaveform = -1;
    GLfloat mSeekingPosLast = 0;

    int mWidth = 100;
    int mHeight = 400;

    QColor backgroudColor;
    QColor waveformColor;
    QColor cursorColor;
    QColor centerLineColor;
    QColor dividerLineColor;
    QColor beatsColor;

    int fullWaveformWidth = 20;
    int waveformPadding = 5;

    void computeOffsetAndScale();

    //returns waveform index
    //or -1 for out of range
    //fills in frame with the frame touched
    int waveformFrame(int& frame, const QPointF& mousePosition) const;
    int frameAtPosition(int waveform, const QPointF& mousePosition) const;
    GLfloat mouseToWaveformPosition(int waveform, const QPointF& mousePosition) const;
  protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
};

#endif // MIXERPANELWAVEFORMSVIEW_H
