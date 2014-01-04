#include "waveformgl.h"
#include <limits>

WaveFormGL::WaveFormGL(QObject * parent) : QObject(parent)
{
  mLines.resize(mWidth);
  mXStartLast = -mLines.size();
}

int WaveFormGL::frameAtX(GLfloat x) const {
  if (!mZoomFull)
    x += (mFramePosition - mHistoryWidth);
  return static_cast<int>(std::roundf(x * mFramesPerLine));
}

void WaveFormGL::setAudioBuffer(djaudio::AudioBufferPtr buffer) {
  mAudioBuffer = buffer;
  if (mZoomFull)
    mFramesPerLine = mAudioBuffer->length() / mWidth;
  mXStartLast = -mLines.size();
  updateLines();
}

void WaveFormGL::setPositionFrame(int frame) {
  mFramePosition = frame;
  if (!mZoomFull)
    updateLines();
}

void WaveFormGL::setWidth(int pixels) {
  mWidth = pixels;
  mLines.resize(mWidth);
  mXStartLast = -mLines.size();
  updateLines();
}

void WaveFormGL::historyWidth(int pixels) {
  mHistoryWidth = pixels;
  mXStartLast = -mLines.size();
  updateLines();
}

void WaveFormGL::draw() {
  if (!mAudioBuffer || mLines.size() == 0)
    return;

  //draw waveform
  glPushMatrix();
  if (!mZoomFull)
    glTranslatef(-((mFramePosition / mFramesPerLine) - mHistoryWidth), 0.0, 0.0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &mLines.front());
  glDrawArrays(GL_TRIANGLES, 0, mLines.size() * 6);
  glDisableClientState(GL_VERTEX_ARRAY);
  glPopMatrix();

  //draw cursor
  GLfloat cursor = mHistoryWidth;
  if (mZoomFull)
    cursor = static_cast<GLfloat>(mFramePosition) / static_cast<GLfloat>(mFramesPerLine);

  glColor4d(cursorColor.redF(), cursorColor.greenF(), cursorColor.blueF(), cursorColor.alphaF());
  glBegin(GL_LINES);
  glVertex2f(cursor, -1.0);
  glVertex2f(cursor, 1.0);
  glEnd();
}

void WaveFormGL::framesPerLine(int v) {
  mFramesPerLine = v;
  updateLines();
}

void WaveFormGL::updateLines() {
  if (!mAudioBuffer)
    return;

  int start_line = 0;
  int start_last = mXStartLast;
  if (!mZoomFull)
    start_line = std::max((mFramePosition / mFramesPerLine) - mHistoryWidth, 0);
  mXStartLast = start_line;
  int end_line = start_line + mLines.size();

  //figure out whats we need to fill in
  if (start_line > start_last) {
    int invalid = std::min(start_line - start_last, mLines.size());
    start_line = end_line - invalid;
  } else {
    int invalid = std::min(start_last - start_line, mLines.size());
    end_line = start_line + invalid;
  }

  for (int i = start_line; i < end_line; i++) {
    GLfloat height = lineHeight(i);
    GLfloat x = i;
    mLines[i % mLines.size()].rect(x, -height, x + 1.0, height);
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

