#include "waveformgl.h"
#include "defines.hpp"
#include <limits>
#include <cmath>

#include <iostream>
using namespace std;

namespace {
  bool registered = false;
  const double pi_over_2 = 1.5707963267948966;
}

WaveFormGL::WaveFormGL(QObject * parent) : QObject(parent)
{
  mWaveformLines.resize(mWidth);
  mWaveformColors.resize(mWidth);

  for (int i = 0; i < mWaveformColors.size(); i++)
    mWaveformColors[i].set(mWaveformColor);
  mXStartLast = -mWaveformLines.size();
  mWaveformCalculator = new WavedataCalculator();
  mCalculateThread = new QThread(this);
  mWaveformCalculator->moveToThread(mCalculateThread);

  if (!registered) {
    qRegisterMetaType<gl2triangles_t>("gl2triangles_t");
    registered = true;
  }

  connect(mWaveformCalculator, &WavedataCalculator::lineChanged, this, &WaveFormGL::setWaveformLine);
  connect(this, &WaveFormGL::waveformLinesRequested, mWaveformCalculator, &WavedataCalculator::compute);
  connect(mWaveformCalculator, &WavedataCalculator::colorChanged, this, &WaveFormGL::setColor);
  connect(this, &WaveFormGL::colorsRequested, mWaveformCalculator, &WavedataCalculator::computeColors);

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
  mBeatBuffer = buffer;
  mBeatLines.clear();
  if (!buffer)
    return;

  if (mZoomFull) {
    for (int i = 0; i < mWaveformColors.size(); i++)
      mWaveformColors[i].set(mWaveformColor);
    emit(colorsRequested(buffer, mWaveformColors.size(), mFramesPerLine));
  } else {
    mBeatLines.resize(buffer->size());
    for (unsigned int i = 0; i < buffer->size(); i++) {
      GLfloat x = static_cast<GLfloat>(buffer->at(i)) / mFramesPerLine;
      mBeatLines[i].x0 = mBeatLines[i].x1 = x;
      mBeatLines[i].y0 = -1;
      mBeatLines[i].y1 = 1;
    }
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
  mWaveformColors.resize(mWidth);
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
  if (!mZoomFull) {
    glColor4d(mWaveformColor.redF(), mWaveformColor.greenF(), mWaveformColor.blueF(), mWaveformColor.alphaF());
    glTranslatef(-((mFramePosition / mFramesPerLine) - mHistoryWidth), 0.0, 0.0);
    glEnableClientState(GL_VERTEX_ARRAY);
  } else {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3, GL_FLOAT, 0, &mWaveformColors.front());
  }
  glVertexPointer(3, GL_FLOAT, 0, &mWaveformLines.front());
  glDrawArrays(GL_TRIANGLES, 0, mWaveformLines.size() * 6);
  glDisableClientState(GL_VERTEX_ARRAY);
  if (mZoomFull)
    glDisableClientState(GL_COLOR_ARRAY);
  
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

  if (mMarkers.size()) {
    glPushMatrix();
    glLineWidth(mZoomFull ? 1.0 : 2.0);
    for (marker_t m: mMarkers.values()) {
      glBegin(GL_LINES);
      GLfloat x0 = static_cast<GLfloat>(m.frame_start) / static_cast<GLfloat>(mFramesPerLine);
      if (m.frame_start == m.frame_end) {
        glColor4d(mMarkerColorJump.redF(), mMarkerColorJump.greenF(), mMarkerColorJump.blueF(), mMarkerColorJump.alphaF());
        glVertex2f(x0, -1.0);
        glVertex2f(x0, 1.0);
      } else {
        if (m.label.length())
          glColor4d(mMarkerColorLoop.redF(), mMarkerColorLoop.greenF(), mMarkerColorLoop.blueF(), mMarkerColorLoop.alphaF());
        else
          glColor4d(mMarkerColorLoopImmediate.redF(), mMarkerColorLoopImmediate.greenF(), mMarkerColorLoopImmediate.blueF(), mMarkerColorLoopImmediate.alphaF());
        GLfloat x1 = static_cast<GLfloat>(m.frame_end) / static_cast<GLfloat>(mFramesPerLine);
        GLfloat y = 1.0;
        glVertex2f(x0, -y);
        glVertex2f(x0, y);

        glVertex2f(x0, y);
        glVertex2f(x1, y);

        glVertex2f(x1, y);
        glVertex2f(x1, -y);

        glVertex2f(x1, -y);
        glVertex2f(x0, -y);
      }
      glEnd();
    }
    glPopMatrix();
  }

  glPopMatrix();

  //draw cursor
  GLfloat cursor = mHistoryWidth;
  if (mZoomFull) {
    cursor = static_cast<GLfloat>(mFramePosition) / static_cast<GLfloat>(mFramesPerLine);
  }

  //dim the played section of the waveform
  glPushMatrix();
  glLineWidth(1.0);
  glEnable(GL_BLEND); //Enable blending.
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glColor4d(0.0f, 0.0f, 0.0f, 0.6f);
  glBegin(GL_TRIANGLES);

  glVertex2f(0.0, -1.0);
  glVertex2f(0.0, 1.0);
  glVertex2f(cursor, 1.0);

  glVertex2f(cursor, 1.0);
  glVertex2f(cursor, -1.0);
  glVertex2f(0.0, -1.0);

  glEnd();
  glPopMatrix();

  glColor4d(cursorColor.redF(), cursorColor.greenF(), cursorColor.blueF(), cursorColor.alphaF());
  glLineWidth(2.0);
  glBegin(GL_LINES);
  glVertex2f(cursor, -1.0);
  glVertex2f(cursor, 1.0);
  glEnd();
}

void WaveFormGL::drawText(QPainter * painter, float width_scale) {
  if (mMarkers.size() == 0)
    return;

  if (!mZoomFull) {
    for (marker_t m: mMarkers.values()) {
      if (m.label.length() == 0)
        continue;
      painter->drawText(0.0,
          width_scale * (mWidth - mHistoryWidth - (static_cast<float>(m.frame_start - mFramePosition) / static_cast<float>(mFramesPerLine))), 40, 40,
          Qt::AlignLeft | Qt::TextWordWrap, m.label);
    }
  } else {
    for (marker_t m: mMarkers.values()) {
      if (m.label.length() == 0)
        continue;
      painter->drawText(0.0, width_scale * static_cast<float>(m.frame_start) / static_cast<float>(mFramesPerLine), 40, 40,
          Qt::AlignLeft | Qt::TextWordWrap, m.label);
    }
  }
}

void WaveFormGL::updateMarker(dj::loop_and_jump_type_t type, int entry, int start, int end) {
  marker_t marker;
  switch (type) {
    case dj::loop_and_jump_type_t::LOOP_BEAT:
    case dj::loop_and_jump_type_t::JUMP_BEAT:
      {
        if (!mBeatBuffer) {
          std::cerr << "no beat buffer to convert marker, skipping" << std::endl;
          return;
        }
        int s = static_cast<int>(mBeatBuffer->size());
        if (s <= start || s <= end || start < 0 || end < 0) {
          std::cerr << "beat index out of range[" << s << "]: " << start << " " << end << std::endl;
          return;
        }
        start = mBeatBuffer->at(start);
        end = mBeatBuffer->at(end);
      }
      break;
    default:
      break;
  }
  marker.frame_start = start;
  marker.frame_end = end;

  if (entry >= 0)
    marker.label = QString::number(entry + 1);
  else
    marker.label = QString();
  mMarkers[entry] = marker;
}

void WaveFormGL::clearMarker(int entry) {
  auto it = mMarkers.find(entry);
  if (it != mMarkers.end())
    mMarkers.erase(it);
}

void WaveFormGL::clearAllMarkers() {
  mMarkers.clear();
}

void WaveFormGL::setBeatLines(QVector<glline_t> lines) {
  mBeatLines = lines;
}

void WaveFormGL::setWaveformLine(int lineIndex, gl2triangles_t value) {
  if (mWaveformLines.size())
    mWaveformLines[lineIndex % mWaveformLines.size()] = value;
}

void WaveFormGL::setColor(int lineIndex, QColor color) {
  if (mWaveformColors.size() < lineIndex)
    return;
  mWaveformColors[lineIndex].set(color);
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
  QColor color_interp(const QColor& start, const QColor& end, double dist) {
   if (dist <= 0.0)
    return start;
   else if (dist >= 1.0)
    return end;

   /*
   QColor r;
   double h,s,v;
   double s_h, s_s, s_v;
   double e_h, e_s, e_v;
   start.getHsvF(&s_h, &s_s, &s_v);
   end.getHsvF(&e_h, &e_s, &e_v);
   s_h = dj::clamp(s_h, 0.0, 1.0);
   s_s = dj::clamp(s_s, 0.0, 1.0);
   s_v = dj::clamp(s_v, 0.0, 1.0);

   e_h = dj::clamp(e_h, 0.0, 1.0);
   e_s = dj::clamp(e_s, 0.0, 1.0);
   e_v = dj::clamp(e_v, 0.0, 1.0);

   h = dj::linear_interp(s_h, e_h, dist);
   s = dj::linear_interp(s_s, e_s, dist);
   v = dj::linear_interp(s_v, e_v, dist);
   */

   QColor r;
   double h;
   double s_h = start.hueF();
   double e_h = end.hueF();

   s_h = dj::clamp(s_h, 0.0, 1.0);
   e_h = dj::clamp(e_h, 0.0, 1.0);

   if (fabs(s_h - e_h) <= 0.5) {
     h = dj::linear_interp(s_h, e_h, dist);
   } else {
     if (s_h > e_h)
       e_h += 1.0;
     else
       s_h += 1.0;
     h = dj::linear_interp(s_h, e_h, dist);
     if (h > 1.0)
       h -= 1.0;
   }

   r.setHsvF(h, 1.0, 1.0);
   return r;
  }

}

WavedataCalculator::WavedataCalculator(QObject * parent) :
  QObject(parent)
{
}

void WavedataCalculator::compute(djaudio::AudioBufferPtr buffer, int startLine, int endLine, int framesPerLine) {
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

void WavedataCalculator::computeColors(djaudio::BeatBufferPtr beats, int lines, int framesPerLine) {
  if (!beats)
    return;
  std::deque<int> dist = beats->distances();
  std::deque<double> off;
  int median = djaudio::median(dist);
  
  //XXX assumes sample rate
  for (unsigned int i = 0; i < dist.size(); i++) {
    double v = 20.0 * dj::clamp(static_cast<double>(dist[i] - median) / 44100.0, -1.0, 1.0);
    //tend to more quickly go away from zero
    v = dj::clamp(sin(v * pi_over_2), -1.0, 1.0);
    off.push_back(v);
  }

  unsigned int beat_index = 0;
  double off_last = 1.0;
  for (int i = 0; i < lines; i++) {
    const int frame = i * framesPerLine;
    double line_off = 0.0;
    int num_beats = 0;
    while(beat_index + 1 < beats->size() && beats->at(beat_index) < frame) {
      if (fabs(off[beat_index]) > fabs(line_off))
        line_off = off[beat_index];
      beat_index++;
      num_beats++;
    }
    if (num_beats == 0)
      line_off = off_last;
    emit(colorChanged(i, color_interp(mWaveformColorOffSlow, mWaveformColorOffFast, (line_off / 2.0) + 0.5)));
    off_last = line_off;
  }
}

