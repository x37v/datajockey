#include "waveformgl.h"
#include <limits>

namespace {
  bool registered = false;
}

WaveFormGL::WaveFormGL(QObject * parent) : QObject(parent)
{
  mWaveformLines.resize(mWidth);
  mXStartLast = -mWaveformLines.size();
  mWaveformCalculator = new WaveformLineCalculator();
  mCalculateThread = new QThread(this);
  mWaveformCalculator->moveToThread(mCalculateThread);

  if (!registered) {
    qRegisterMetaType<gl2triangles_t>("gl2triangles_t");
    registered = true;
  }

  connect(mWaveformCalculator, SIGNAL(lineChanged(int, gl2triangles_t)),
      this, SLOT(setWaveformLine(int, gl2triangles_t)));
  connect(this, SIGNAL(waveformLinesRequested(djaudio::AudioBufferPtr, int, int, int)),
      mWaveformCalculator, SLOT(compute(djaudio::AudioBufferPtr, int, int, int)));

  mCalculateThread->start();
}

int WaveFormGL::frameAtX(GLfloat x) const {
  if (!mZoomFull)
    x += (mFramePosition - mHistoryWidth);
  return static_cast<int>(std::roundf(x * mFramesPerLine));
}

void WaveFormGL::setAudioBuffer(djaudio::AudioBufferPtr buffer) {
  mAudioBuffer = buffer;
  bzero(&mWaveformLines.front(), mWaveformLines.size() * sizeof(gl2triangles_t));
  if (!buffer)
    return;

  if (mZoomFull)
    mFramesPerLine = mAudioBuffer->length() / mWidth;
  mXStartLast = -mWaveformLines.size();
  updateLines();
}
 
void WaveFormGL::setBeatBuffer(djaudio::BeatBufferPtr buffer) {
  mBeatLines.clear();
  if (mZoomFull || !buffer)
    return;

  mBeatLines.resize(buffer->size());
  for (unsigned int i = 0; i < buffer->size(); i++) {
    GLfloat x = static_cast<GLfloat>(buffer->at(i)) / mFramesPerLine;
    mBeatLines[i].x0 = mBeatLines[i].x1 = x;
    mBeatLines[i].y0 = -1;
    mBeatLines[i].y1 = 1;
  }
}

void WaveFormGL::setPositionFrame(int frame) {
  mFramePosition = frame;
  if (!mZoomFull)
    updateLines();
}

void WaveFormGL::setWidth(int pixels) {
  mWidth = pixels;
  mWaveformLines.resize(mWidth);
  mXStartLast = -mWaveformLines.size();
  updateLines();
}

void WaveFormGL::historyWidth(int pixels) {
  mHistoryWidth = pixels;
  mXStartLast = -mWaveformLines.size();
  updateLines();
}

void WaveFormGL::draw() {
  if (!mAudioBuffer || mWaveformLines.size() == 0)
    return;

  //draw waveform
  glPushMatrix();
  glColor4d(mWaveformColor.redF(), mWaveformColor.greenF(), mWaveformColor.blueF(), mWaveformColor.alphaF());
  if (!mZoomFull)
    glTranslatef(-((mFramePosition / mFramesPerLine) - mHistoryWidth), 0.0, 0.0);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 0, &mWaveformLines.front());
  glDrawArrays(GL_TRIANGLES, 0, mWaveformLines.size() * 6);
  glDisableClientState(GL_VERTEX_ARRAY);
  
  if (mBeatLines.size()) {
    //draw beats
    glPushMatrix();
    glColor4d(mBeatColor.redF(), mBeatColor.greenF(), mBeatColor.blueF(), mBeatColor.alphaF());
    glLineWidth(1.0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, &mBeatLines.front());
    glDrawArrays(GL_LINES, 0, mBeatLines.size() * 2);
    glDisableClientState(GL_VERTEX_ARRAY);
    glPopMatrix();
  }

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

void WaveFormGL::setBeatLines(QVector<glline_t> lines) {
  mBeatLines = lines;
}

void WaveFormGL::setWaveformLine(int lineIndex, gl2triangles_t value) {
  if (mWaveformLines.size())
    mWaveformLines[lineIndex % mWaveformLines.size()] = value;
}

void WaveFormGL::framesPerLine(int v) {
  mFramesPerLine = v;
  updateLines();
}

void WaveFormGL::updateLines() {
  if (!mAudioBuffer)
    return;

  if (mZoomFull) {
    emit(waveformLinesRequested(mAudioBuffer, 0, mWaveformLines.size(), mFramesPerLine));
    return;
  }

  int start_last = mXStartLast;
  int start_line = std::max((mFramePosition / mFramesPerLine) - mHistoryWidth, 0);
  mXStartLast = start_line;
  int end_line = start_line + mWaveformLines.size();

  //figure out whats we need to fill in
  if (start_line > start_last) {
    int invalid = std::min(start_line - start_last, mWaveformLines.size());
    start_line = end_line - invalid;
  } else {
    int invalid = std::min(start_last - start_line, mWaveformLines.size());
    end_line = start_line + invalid;
  }

  emit(waveformLinesRequested(mAudioBuffer, start_line, end_line, mFramesPerLine));
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

namespace {
  GLfloat lineHeight(djaudio::AudioBufferPtr buffer, int line_index, int frames_per_line) {
    //this is only called with a valid audio buffer
    int start_frame = line_index * frames_per_line;

    if (start_frame < 0 || start_frame >= (int)buffer->length())
      return (GLfloat)0.0;

    int end_frame = std::min(start_frame + frames_per_line, (int)buffer->length());
    float value = 0;
    for (int frame = start_frame; frame < end_frame; frame++) {
      value = std::max(value, fabsf(buffer->sample(0, frame)));
      value = std::max(value, fabsf(buffer->sample(1, frame)));
    }
    return (GLfloat)value;
  }
}

WaveformLineCalculator::WaveformLineCalculator(QObject * parent) :
  QObject(parent)
{
}

void WaveformLineCalculator::compute(djaudio::AudioBufferPtr buffer, int startLine, int endLine, int framesPerLine) {
  if (!buffer)
    return;

  for (int i = startLine; i < endLine; i++) {
    GLfloat height = lineHeight(buffer, i, framesPerLine);
    GLfloat x = i;

    gl2triangles_t triangle;
    triangle.rect(x, -height, x + 1.0, height);
    emit(lineChanged(i, triangle));
  }
}

