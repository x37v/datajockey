#include "waveformviewgl.hpp"
#include <QtGui>
#include <stdlib.h>

#define NUM_VERTICIES 800

WaveFormViewGL::WaveFormViewGL(QWidget * parent) :
   QGLWidget(parent),
   mHeight(100),
   mWidth(400),
   mVertical(false),
   mColorBackgroud(QColor::fromRgb(0,0,0)),
   mColorWaveform(QColor::fromRgb(255,0,0).dark()),
   mColorCursor(QColor::fromRgb(0,255,0)),
   mColorCenterLine(QColor::fromRgb(0,0,255))
{
   mVerticies.resize(2 * NUM_VERTICIES);

   for(unsigned int i = 0; i < mVerticies.size(); i += 4) {
      GLfloat v = ((GLfloat)rand() / (GLfloat)RAND_MAX);
      mVerticies[i] = mVerticies[i + 2] = (GLfloat)(i / 4);
      mVerticies[i + 1] = v;
      mVerticies[i + 3] = -v;
   }
}

QSize WaveFormViewGL::minimumSizeHint() const { return QSize(100, 100); }
QSize WaveFormViewGL::sizeHint() const { return mVertical ? QSize(100, 400) : QSize(400, 100); }

void WaveFormViewGL::initializeGL(){
   qglClearColor(mColorBackgroud);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, mWidth, mHeight, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);
   glDisable(GL_DEPTH_TEST);
}

void WaveFormViewGL::paintGL(){
   glClear(GL_COLOR_BUFFER_BIT);

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

   //draw waveform
   qglColor(mColorWaveform);
   glLineWidth(1.0);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, &mVerticies.front());
   glDrawArrays(GL_LINES, 0, mVerticies.size());

   //draw cursor
   qglColor(mColorCursor);
   glLineWidth(2.0);
   glBegin(GL_LINES);
   glVertex2f(100, -1.);
   glVertex2f(100, 1.0);
   glEnd();

   //draw center line
   qglColor(mColorCenterLine);
   glLineWidth(1.0);
   glBegin(GL_LINES);
   glVertex2f(0, 0);
   glVertex2f(mVertical ? mHeight : mWidth, 0);
   glEnd();
}

void WaveFormViewGL::resizeGL(int width, int height){
   mWidth = width;
   mHeight = height;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, mWidth, mHeight, 0, 0, 1);
   glMatrixMode(GL_MODELVIEW);
   glDisable(GL_DEPTH_TEST);

   glViewport(0, 0, mWidth, mHeight);
}

/*
void WaveFormViewGL::mousePressEvent(QMouseEvent *event){
}

void WaveFormViewGL::mouseMoveEvent(QMouseEvent *event){
}
*/
