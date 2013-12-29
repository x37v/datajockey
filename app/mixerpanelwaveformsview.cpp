#include "mixerpanelwaveformsview.h"
#include "waveformgl.h"

MixerPanelWaveformsView::MixerPanelWaveformsView(QWidget *parent) :
  QGLWidget(parent)
{
  for (int i = 0; i < mNumPlayers * 2; i++)
    mWaveforms.push_back(new WaveFormGL);
}

void MixerPanelWaveformsView::playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (player >= mNumPlayers || player < 0)
    return;

  //2 waveforms per player
  player *= 2;
  mWaveforms[player]->setAudioBuffer(audio_buffer);
  mWaveforms[player + 1]->setAudioBuffer(audio_buffer);
}

void MixerPanelWaveformsView::initializeGL() {
  qglClearColor(Qt::black); //background color

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, mWidth, mHeight, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_MULTISAMPLE);
  glDisable(GL_DEPTH_TEST);
}

void MixerPanelWaveformsView::paintGL() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  //we a drawing vertically but pretending that it is horizontal
  const GLfloat width = mHeight;
  const GLfloat height = mWidth;

  //XXX just drawing first
  WaveFormGL * wf = mWaveforms[0];

  glPushMatrix();

  //draw vertical
  //set up 0, 0 to be at the top center of the view
  //rotate so that we draw as if horizontal but actually happens vertical
  glTranslatef((GLfloat)mWidth / 2, 0.0, 0.0);
  glRotatef(90.0, 0.0, 0.0, 1.0);

  glPushMatrix();
  //glScalef(1.0, 1.0 / mWidth, 1.0);
  wf->draw();
  glPushMatrix();

  //draw center line
  qglColor(Qt::white);
  glLineWidth(1.0);
  glBegin(GL_LINES);
  glVertex2f(2.0, 0.0);
  glVertex2f(width - 2.0, 0.0);
  glEnd();

  glPopMatrix();
  glFlush();
}

void MixerPanelWaveformsView::resizeGL(int width, int height) {
  mWidth = width;
  mHeight = height;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, mWidth, mHeight, 0, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glDisable(GL_DEPTH_TEST);

  glViewport(0, 0, mWidth, mHeight);
}

