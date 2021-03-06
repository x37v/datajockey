#include "mixerpanelwaveformsview.h"
#include "waveformgl.h"
#include <iostream>

using std::cout;
using std::endl;

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

namespace {
  const GLfloat divider_z = 0.5;
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

  mWaveforms[1]->zoomFull(false);
  mWaveforms[2]->zoomFull(false);

  computeOffsetAndScale();

  setAutoFillBackground(false);
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
}

void MixerPanelWaveformsView::playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (player >= mNumPlayers || player < 0)
    return;

  //2 waveforms per player
  player *= 2;
  mWaveforms[player]->setAudioBuffer(audio_buffer);
  mWaveforms[player]->setBeatBuffer(beat_buffer);
  mWaveforms[player + 1]->setAudioBuffer(audio_buffer);
  mWaveforms[player + 1]->setBeatBuffer(beat_buffer);
  update();
}

void MixerPanelWaveformsView::playerSetValueInt(int player, QString name, int v) {
  if (player >= mNumPlayers || player < 0 || name != "position_frame")
    return;
  player *= 2;
  mWaveforms[player]->setPositionFrame(v);
  mWaveforms[player + 1]->setPositionFrame(v);
  update();
}

void MixerPanelWaveformsView::updateMarker(int player, dj::loop_and_jump_type_t type, int entry, int start, int end) {
  if (player >= mNumPlayers || player < 0)
    return;
  player *= 2;
  mWaveforms[player]->updateMarker(type, entry, start, end);
  mWaveforms[player + 1]->updateMarker(type, entry, start, end);
  update();
}

void MixerPanelWaveformsView::clearMarker(int player, int entry) {
  if (player >= mNumPlayers || player < 0)
    return;
  player *= 2;
  mWaveforms[player]->clearMarker(entry);
  mWaveforms[player + 1]->clearMarker(entry);
  update();
}

void MixerPanelWaveformsView::clearAllMarkers(int player) {
  if (player >= mNumPlayers || player < 0)
    return;
  player *= 2;
  mWaveforms[player]->clearAllMarkers();
  mWaveforms[player + 1]->clearAllMarkers();
  update();
}

void MixerPanelWaveformsView::initializeGL() {
}

//void MixerPanelWaveformsView::paintGL() {
void MixerPanelWaveformsView::paintEvent(QPaintEvent * event) {
  QPainter painter(this);

  painter.setBackgroundMode(Qt::OpaqueMode);
  painter.setBackground(QBrush(Qt::black));

  glLoadIdentity();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (mWidth <= 0 || mHeight <= 0)
    return;

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
    WaveFormGL * wf = mWaveforms[i];
    glPushMatrix();
    glTranslatef(0.0, mOffsetAndScale[i].first, 0.0);

    //draw waveform
    glPushMatrix();

    //if we aren't in full zoom, draw from the bottom
    if (!wf->zoomFull()) {
      glTranslatef(mHeight, 0.0, 0.0);
      glRotatef(180.0, 0.0, 0.0, 1.0);
    }
    //scale it so that the whole waveform fits in the view ["x"] and its height is set by mOffsetAndScale
    GLfloat height_scale = static_cast<GLfloat>(mHeight) / static_cast<GLfloat>(wf->width());
    glScalef(height_scale, mOffsetAndScale[i].second, 1.0);
    glLineWidth(1.0);
    wf->draw();

    glPopMatrix();

    //draw center line
    glBegin(GL_LINES);
    glLineWidth(1.0);
    qglColor(centerLineColor);
    glVertex3f(0.0, 0.0, divider_z);
    glVertex3f(mHeight, 0.0, divider_z);
    glEnd();

    glPopMatrix();
  }

  glPopMatrix();
  //glFlush();
  
  painter.resetTransform();
  painter.translate(QPointF(mWidth / 2, 0.0));
  for (int i = 0; i < mWaveforms.size(); i++) {
    WaveFormGL * wf = mWaveforms[i];
    painter.save();
    //draw at the left of the waveform
    painter.translate(QPointF(mOffsetAndScale[i].first - mOffsetAndScale[i].second, 0.0));

    //GLfloat height_scale = static_cast<GLfloat>(mHeight) / static_cast<GLfloat>(wf->width());
    //painter.scale(height_scale, mOffsetAndScale[i].second);
    
    GLfloat height_scale = static_cast<GLfloat>(mHeight) / static_cast<GLfloat>(wf->width());
    //painter.scale(height_scale, mOffsetAndScale[i].second);

    mWaveforms[i]->drawText(&painter, height_scale);
    painter.restore();
  }
  
  /*
  painter.setRenderHint(QPainter::TextAntialiasing);
  painter.setPen(Qt::white);
  painter.drawText(0, 0, 100, 100, Qt::AlignLeft | Qt::TextWordWrap, "TEST");
  painter.end();
  */
}

void MixerPanelWaveformsView::resizeGL(int width, int height) {
  mWidth = width;
  mHeight = height;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, mWidth, mHeight, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glEnable(GL_MULTISAMPLE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND); //Enable blending.
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport(0, 0, mWidth, mHeight);
  computeOffsetAndScale();
  qglClearColor(backgroudColor); //background color
}

void MixerPanelWaveformsView::mouseMoveEvent(QMouseEvent * event) {
  if (mSeekingWaveform < 0)
    return;

  //we don't care which waveform we are on, since we are seeking..
  GLfloat pos = mouseToWaveformPosition(mSeekingWaveform, event->localPos());
  emit(playerValueChangedInt(mSeekingWaveform / 2, "seek_frame_relative", static_cast<int>((mSeekingPosLast - pos) * static_cast<GLfloat>(mWaveforms[mSeekingWaveform]->framesPerLine()))));
  mSeekingPosLast = pos;
}

void MixerPanelWaveformsView::mousePressEvent(QMouseEvent * event) {
  int frame;
  int waveform = waveformFrame(frame, event->localPos());
  if (waveform < 0)
    return;
  int player = waveform / 2;
  if (mWaveforms[waveform]->zoomFull())
    emit(playerValueChangedInt(player, "seek_frame", frame));
  else {
    mSeekingWaveform = waveform;
    mSeekingPosLast = mouseToWaveformPosition(waveform, event->localPos());
    emit(playerValueChangedBool(mSeekingWaveform / 2, "seeking", true));
  }
}

void MixerPanelWaveformsView::mouseReleaseEvent(QMouseEvent * /* event */) {
  if (mSeekingWaveform < 0)
    return;
  emit(playerValueChangedBool(mSeekingWaveform / 2, "seeking", false));
  mSeekingWaveform = -1;
}

int MixerPanelWaveformsView::waveformFrame(int& frame, const QPointF& mousePosition) const {
  GLfloat x = mousePosition.x() - (mWidth / 2);
  frame = 0;

  for (int i = 0; i < mOffsetAndScale.size(); i++) {
    GLfloat center = mOffsetAndScale[i].first;
    GLfloat range[2] = {center - mOffsetAndScale[i].second, center + mOffsetAndScale[i].second};
    if (x < range[1] && x > range[0]) {
      frame = frameAtPosition(i, mousePosition);
      return i;
    }
  }

  return -1;
}

int MixerPanelWaveformsView::frameAtPosition(int waveform, const QPointF& mousePosition) const {
  return static_cast<int>(std::roundf(mWaveforms[waveform]->frameAtX(mouseToWaveformPosition(waveform, mousePosition))));
}

GLfloat MixerPanelWaveformsView::mouseToWaveformPosition(int waveform, const QPointF& mousePosition) const {
  GLfloat y = mousePosition.y();
  WaveFormGL * wf = mWaveforms[waveform];
  //non full view starts at bottom
  if (!wf->zoomFull())
    y = mHeight - y;
  //scale to waveform size
  return (y * static_cast<GLfloat>(wf->width())) / static_cast<GLfloat>(mHeight);
}

