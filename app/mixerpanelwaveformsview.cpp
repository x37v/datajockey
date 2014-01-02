#include "mixerpanelwaveformsview.h"
#include "waveformgl.h"
#include <iostream>

using std::cout;
using std::endl;

namespace {
  const GLfloat divider_z = 1.0;
}

MixerPanelWaveformsView::MixerPanelWaveformsView(QWidget *parent) :
  QGLWidget(parent),
  backgroudColor(Qt::black),
  waveformColor(Qt::darkRed),
  cursorColor(Qt::blue),
  centerLineColor(Qt::green),
  dividerLineColor(Qt::white)
{
  for (int i = 0; i < mNumPlayers * 2; i++) {
    mWaveforms.push_back(new WaveFormGL);
    mOffsetAndScale.push_back(QPair<GLfloat, GLfloat>());
  }
  computeOffsetAndScale();
}

//full - pad - zoomed - pad pad - zoomed - pad - full
void MixerPanelWaveformsView::computeOffsetAndScale() {
  int pixels = mWidth / 2 - (waveformPadding * 2);
  int zoomed = pixels - fullWaveformWidth;

  mOffsetAndScale[0].second = mOffsetAndScale[3].second =
    static_cast<GLfloat>(fullWaveformWidth / 2);
  mOffsetAndScale[1].second = mOffsetAndScale[2].second =
    static_cast<GLfloat>(zoomed / 2);

  mOffsetAndScale[2].first = static_cast<GLfloat>(waveformPadding + zoomed / 2);
  mOffsetAndScale[3].first = static_cast<GLfloat>(waveformPadding + zoomed + waveformPadding + fullWaveformWidth / 2);

  mOffsetAndScale[1].first = -mOffsetAndScale[2].first;
  mOffsetAndScale[0].first = -mOffsetAndScale[3].first;

  cout << "offset -> scale" << endl;
  for (auto os: mOffsetAndScale) {
    cout << os.first << " : " << os.second << endl;
  }
}

void MixerPanelWaveformsView::playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (player >= mNumPlayers || player < 0)
    return;

  //2 waveforms per player
  player *= 2;
  mWaveforms[player]->setAudioBuffer(audio_buffer);
  mWaveforms[player + 1]->setAudioBuffer(audio_buffer);
  update();
}

void MixerPanelWaveformsView::initializeGL() {
  qglClearColor(backgroudColor); //background color

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

  glPushMatrix();

  //draw vertical
  //set up 0, 0 to be at the top center of the view
  //rotate so that we draw as if horizontal but actually happens vertical
  glTranslatef((GLfloat)mWidth / 2, 0.0, 0.0);
  glRotatef(90.0, 0.0, 0.0, 1.0);
  glRotatef(180.0, 1.0, 0.0, 0.0);

  //draw divider line
  qglColor(dividerLineColor);
  glLineWidth(1.0);
  glBegin(GL_LINES);
  glVertex3f(2.0, 0.0, divider_z);
  glVertex3f(mHeight - 2.0, 0.0, divider_z);
  glEnd();

  //draw waveforms
  for (int i = 0; i < mWaveforms.size(); i++) {
    glPushMatrix();
    glTranslatef(0.0, mOffsetAndScale[i].first, 0.0);

    //draw center line
    glBegin(GL_LINES);
    qglColor(centerLineColor);
    glVertex3f(0.0, 0.0, divider_z);
    glVertex3f(mHeight, 0.0, divider_z);
    glEnd();

    //draw waveform
    glScalef(1.0, mOffsetAndScale[i].second, 1.0);
    qglColor(waveformColor);
    mWaveforms[i]->draw();

    glPopMatrix();
  }

  glPopMatrix();
  //glFlush();
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
  computeOffsetAndScale();
}

