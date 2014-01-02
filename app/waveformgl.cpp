#include "waveformgl.h"

WaveFormGL::WaveFormGL(QObject * parent) : QObject(parent)
{
}

void WaveFormGL::setAudioBuffer(djaudio::AudioBufferPtr buffer) {
  mAudioBuffer = buffer;
  updateLines();
}

void WaveFormGL::setPositionFrame(int frame) {
}

void WaveFormGL::setWidth(int pixels) {
  mWidth = pixels;
  updateLines();
}

void WaveFormGL::draw() {
  if (!mAudioBuffer || mLines.size() == 0)
    return;

  //draw waveform
  glPushMatrix();
  if (mZoomFull) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &mLines.front());
    glDrawArrays(GL_TRIANGLES, 0, mLines.size() * 6);
    glDisableClientState(GL_VERTEX_ARRAY);
  }
  glPopMatrix();
}

void WaveFormGL::updateLines() {
  if (!mAudioBuffer)
    return;

  if (mZoomFull) {
    mFramesPerLine = mAudioBuffer->length() / mWidth;
    mLines.resize(mWidth);
    for (int i = 0; i < mWidth; i++) {
      GLfloat height = lineHeight(i);
      GLfloat x = i;
      mLines[i].rect(x, -height, x + 1.0, height);
    }
  }
}

GLfloat WaveFormGL::lineHeight(int line_index) const {
  //this is only called with a valid audio buffer
  int start_frame = line_index * mFramesPerLine;

  if (start_frame < 0 || start_frame >= (int)mAudioBuffer->length())
    return (GLfloat)0.0;

  int end_frame = std::min(start_frame + mFramesPerLine, (int)mAudioBuffer->length());
  float value = 0;
  for (int frame = start_frame; frame < end_frame; frame++) {
    value = std::max(value, fabsf(mAudioBuffer->sample(0, frame)));
    value = std::max(value, fabsf(mAudioBuffer->sample(1, frame)));
  }
  return (GLfloat)value;
}

