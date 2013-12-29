#ifndef MIXERPANELWAVEFORMSVIEW_H
#define MIXERPANELWAVEFORMSVIEW_H

#include <QWidget>
#include <QGLWidget>
#include <QtOpenGL>
#include "audiobuffer.hpp"
#include "beatbuffer.hpp"

class WaveFormGL;

class MixerPanelWaveformsView : public QGLWidget
{
  Q_OBJECT
  public:
    explicit MixerPanelWaveformsView(QWidget *parent = 0);

  signals:

  public slots:
    void playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);
  private:
    QList<WaveFormGL *> mWaveforms;
    int mNumPlayers = 2;

    int mWidth = 100;
    int mHeight = 400;
  protected:
    virtual void initializeGL();
    virtual void paintGL();
    virtual void resizeGL(int width, int height);
};

#endif // MIXERPANELWAVEFORMSVIEW_H
