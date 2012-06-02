#include "waveformviewgl.hpp"
#include "audiobuffer.hpp"
#include <QtGui>
#include <QMutexLocker>
#include <stdlib.h>
#include <algorithm>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif


using namespace dj::view;

WaveFormViewGL::WaveFormViewGL(QWidget * parent, bool vertical) :
   QGLWidget(parent),
   mMutex(),
   mHeight(100),
   mWidth(400),
   mCursorOffset(50),
   mVertical(vertical),
   mFirstLineIndex(0),
   mVerticiesValid(false),
   mBeatBuffer(),
   mBeatVerticies(),
   mBeatVerticiesValid(false),
   mSampleRate(44100.0),
   mFramesPerLine(256),
   mFrame(0),
   mColorBackgroud(QColor::fromRgb(0,0,0)),
   mColorWaveform(QColor::fromRgb(255,0,0).dark()),
   mColorCursor(QColor::fromRgb(0,255,0)),
   mColorCenterLine(QColor::fromRgb(0,0,255)),
   mColorBeats(QColor::fromRgb(255,255,0)),
   mLastMousePos(0)
{
}

QSize WaveFormViewGL::minimumSizeHint() const { return QSize(100, 100); }
QSize WaveFormViewGL::sizeHint() const { return mVertical ? QSize(100, 400) : QSize(400, 100); }
void WaveFormViewGL::setVertical(bool vert) { mVertical = vert; }

void WaveFormViewGL::clear_audio() {
   QMutexLocker lock(&mMutex);
   mAudioBuffer.release();
   mVerticiesValid = false;
   update();
}

void WaveFormViewGL::set_audio_file(QString file_name) { 
   QMutexLocker lock(&mMutex);
   mAudioBuffer.reset(file_name); 
   if (mAudioBuffer.valid()) {
      float sample_rate = mAudioBuffer->sample_rate();
      if (sample_rate != mSampleRate) {
         mSampleRate = sample_rate;
         //if the beat buffer was set before the audio buffer, we'll need to redraw
         if (mBeatVerticiesValid)
            update_beats();
      }
   }
   mVerticiesValid = false;
   update();
}

void WaveFormViewGL::clear_beats() {
   QMutexLocker lock(&mMutex);
   mBeatVerticiesValid = false;
   update();
}
            
void WaveFormViewGL::set_beat_buffer(audio::BeatBuffer & buffer) {
   QMutexLocker lock(&mMutex);
   mBeatBuffer = buffer;
   if (mBeatBuffer.length() > 2) {

      //XXX make configurable
      double frames_per_view = mBeatBuffer.median_difference() * 8.0 * mSampleRate;
      mFramesPerLine = frames_per_view / (mVertical ? mHeight : mWidth);
      mVerticiesValid = false;

      update_beats();
   } else
      mBeatVerticiesValid = false;
   update();
}

void WaveFormViewGL::set_frame(int frame) {
   int prev = mFrame;
   mFrame = frame;
   if (prev > mFrame || mFrame >= prev + mFramesPerLine)
      update();
}

void WaveFormViewGL::set_frames_per_line(int num_frames) {
   if (num_frames < 1)
      num_frames = 1;
   if (mFramesPerLine != num_frames) {
      mVerticiesValid = false;
      mFramesPerLine = num_frames;
      update();
   }
}

void WaveFormViewGL::initializeGL(){
   QMutexLocker lock(&mMutex);

   qglClearColor(mColorBackgroud);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, mWidth, mHeight, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);
   glEnable(GL_MULTISAMPLE);
   glDisable(GL_DEPTH_TEST);

   mVerticies.resize(4 * (mVertical ? mHeight : mWidth));
}

void WaveFormViewGL::paintGL(){
   QMutexLocker lock(&mMutex);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   if (mVertical) {
      glTranslatef(mWidth / 2, mHeight, 0.0);
      glRotatef(-90.0, 0.0, 0.0, 1.0);
      //-1..1 in the y direction
      glScalef(1.0, mWidth / 2, 1.0);
   } else {
      glTranslatef(0.0, mHeight / 2, 0.0);
      //-1..1 in the x direction
      glScalef(1.0, mHeight / 2, 1.0);
   }

   if (mAudioBuffer.valid()) {
      //TODO treat vertices as a circular buffer and only update what we need to
      update_waveform();

      if (mVerticiesValid) {
         //draw waveform
         glPushMatrix();
         glTranslatef(-mVerticies[mFirstLineIndex * 4], 0, 0);
         qglColor(mColorWaveform);
         glLineWidth(1.0);
         glEnableClientState(GL_VERTEX_ARRAY);
         glVertexPointer(2, GL_FLOAT, 0, &mVerticies.front());
         glDrawArrays(GL_LINES, 0, mVerticies.size() / 2);

         //draw beats if we have them
         if (mBeatVerticiesValid) {
            glPushMatrix();
            qglColor(mColorBeats);
            glLineWidth(1.0);
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, &mBeatVerticies.front());
            glDrawArrays(GL_LINES, 0, mBeatVerticies.size() / 2);
            glPopMatrix();
         }

         glPopMatrix();
      }
   }

   //draw cursor
   qglColor(mColorCursor);
   glLineWidth(2.0);
   glBegin(GL_LINES);
   glVertex2f(mCursorOffset, -1.0);
   glVertex2f(mCursorOffset, 1.0);
   glEnd();

   //draw center line
   qglColor(mColorCenterLine);
   glLineWidth(1.0);
   glBegin(GL_LINES);
   glVertex2f(0, 0);
   glVertex2f(mVertical ? mHeight : mWidth, 0);
   glEnd();

   glFlush();
}

void WaveFormViewGL::resizeGL(int width, int height) {
   QMutexLocker lock(&mMutex);

   if (mVertical) {
      if (mHeight != height) {
         mVerticies.resize(4 * height);
         mVerticiesValid = false;
      }
   } else {
      if (mWidth != width) {
         mVerticies.resize(4 * width);
         mVerticiesValid = false;
      }
   }

   mWidth = width;
   mHeight = height;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, mWidth, mHeight, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);
   glDisable(GL_DEPTH_TEST);

   glViewport(0, 0, mWidth, mHeight);
}


void WaveFormViewGL::mouseMoveEvent(QMouseEvent * event) {
   int diff = event->y() - mLastMousePos;
   mLastMousePos = event->y();
   int frames = mFramesPerLine * diff;
   emit(seek_relative(frames));
}

void WaveFormViewGL::mousePressEvent(QMouseEvent * event) {
   mLastMousePos = event->y();
   emit(mouse_down(true));
}

void WaveFormViewGL::mouseReleaseEvent(QMouseEvent * event) {
   emit(mouse_down(false));
}

void WaveFormViewGL::update_waveform() {
   int first_line = ((mFrame / mFramesPerLine) - mCursorOffset);
   const int total_lines = (int)mVerticies.size() / 4;
   int compute_lines = total_lines;
   int compute_line_offset = 0;

   if (mVerticiesValid) {
      //XXX should we just store this value to not have to convert back from a float?
      int current_first_line = (int)mVerticies[mFirstLineIndex * 4];
      if (current_first_line <= first_line) {
         //our new first line is at or after our current first line
         //there is data to keep
         if (current_first_line + total_lines > first_line) {
            int offset = first_line - current_first_line;
            mFirstLineIndex = (mFirstLineIndex + offset) % total_lines;
            //the new data to fill is between first_line and current_first_line
            compute_lines = offset;
            compute_line_offset = total_lines - offset + mFirstLineIndex;
            first_line = current_first_line + total_lines;
         } else {
            //totally wipe out what we have
            mFirstLineIndex = 0;
         }
      } else {
         //our new first line is before our current first line
         //there is data to keep
         if (first_line + total_lines > current_first_line) {
            int offset = current_first_line - first_line;
            mFirstLineIndex -= offset;
            if (mFirstLineIndex < 0)
               mFirstLineIndex += total_lines;
            //the new data to fill is between first_line and current_first_line
            compute_lines = offset;
            compute_line_offset = mFirstLineIndex;
         } else {
            //totally wipe out what we have
            mFirstLineIndex = 0;
         }
      }
   } else {
      if (first_line >= 0) {
         mFirstLineIndex = first_line % total_lines;
      } else {
         mFirstLineIndex = first_line;
         while(mFirstLineIndex < 0)
            mFirstLineIndex += total_lines;
      }
      compute_line_offset = mFirstLineIndex;
   }

   //this is only called with a valid audio buffer
   for(int line = 0; line < compute_lines; line++) {
      int index = ((line + compute_line_offset) % total_lines) * 4;
      int line_index = line + first_line;
      GLfloat value = line_value(line_index);
      mVerticies[index] = mVerticies[index + 2] = line_index;
      mVerticies[index + 1] = value;
      mVerticies[index + 3] = -value;
   }
   mVerticiesValid = true;
}

void WaveFormViewGL::update_beats() {
   mBeatVerticies.resize(4 * mBeatBuffer.length());
   for(unsigned int i = 0; i < mBeatBuffer.length(); i++) {
      int line_index = i * 4;
      GLfloat pos = (mSampleRate * mBeatBuffer[i]) / mFramesPerLine;
      mBeatVerticies[line_index] = mBeatVerticies[line_index + 2] = pos;
      mBeatVerticies[line_index + 1] = static_cast<GLfloat>(1.0);
      mBeatVerticies[line_index + 3] = static_cast<GLfloat>(-1.0);
   }
   mBeatVerticiesValid = true;
}

GLfloat WaveFormViewGL::line_value(int line_index) {
   //this is only called with a valid audio buffer
   int start_frame = line_index * mFramesPerLine;

   if (start_frame < 0 || start_frame >= (int)mAudioBuffer->length()) {
      return (GLfloat)0.0;
   } else {
      int end_frame = std::min(start_frame + mFramesPerLine, (int)mAudioBuffer->length());
      float value = 0;
      for (int frame = start_frame; frame < end_frame; frame++) {
         value = std::max(value, mAudioBuffer->sample(0, frame));
         value = std::max(value, mAudioBuffer->sample(1, frame));
      }
      return (GLfloat)value;
   }
}
