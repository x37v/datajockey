#include "waveformview.hpp"
#include "waveformitem.hpp"
#include "audiomodel.hpp"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QGraphicsLineItem>

using namespace dj::view;

WaveFormView::WaveFormView(QWidget * parent) : mLastMousePos(0), mSampleRate(44100.0) {
   rotate(-90);

   mScene = new QGraphicsScene(this);

   mWaveForm = new WaveFormItem();
   mWaveForm->setZoom(150);
   mWaveForm->setPen(QPen(Qt::red));
   mScene->addItem(mWaveForm);

   mCursor = new QGraphicsLineItem(0, -100, 0, 100);
   mCursor->setPen(QPen(Qt::green));
   mScene->addItem(mCursor);

   this->setScene(mScene);

   setBackgroundBrush(QBrush(Qt::black));

   //mWaveFormView->fitInView(mWaveForm);
   //ensureVisible(mScene->sceneRect(), Qt::IgnoreAspectRatio);
}

WaveFormView::~WaveFormView() {
}

void WaveFormView::set_audio_file(const QString& file_name) {

   mSampleRate = (double)audio::AudioModel::instance()->sample_rate();

   mWaveForm->setAudioFile(file_name);

   //resetMatrix();
   //scale((float)geometry().width() / 200.0, 1.0);
   //rotate(-90);
   //mScene->setSceneRect(mWaveForm->boundingRect());

   QRectF rect = mWaveForm->boundingRect();
   //rect.setWidth(rect.width() + 2 * geometry().height());
   //rect.setX(rect.x() - geometry().height());
   mScene->setSceneRect(rect);

}

void WaveFormView::set_beat_buffer(audio::BeatBufferPtr buffer) {
   mBeatBuffer = buffer;
   int index = 0;
   if (mBeatBuffer) {
      //update the current ones
      for (; index < std::min((int)mBeatBuffer->length(), mBeatLines.length()); index++) {
         qreal pos = (mBeatBuffer->at(index) * mSampleRate) / mWaveForm->zoom();
         mBeatLines[index]->setLine(pos, -100, pos, 100);
         mBeatLines[index]->setVisible(true);
      }
      //allocate any new that we need
      for (; index < (int)mBeatBuffer->length(); index++) {
         qreal pos = (mBeatBuffer->at(index) * mSampleRate) / mWaveForm->zoom();
         mBeatLines.append(mScene->addLine(pos, -100, pos, 100, QPen(Qt::yellow)));
      }
   }
   //set the unneeded ones to invisible
   for (; index < mBeatLines.length(); index++) {
      mBeatLines[index]->setVisible(false);
   }
}

void WaveFormView::set_audio_frame(int frame) {
   int offset = geometry().height() / 4;
   centerOn(frame / mWaveForm->zoom() + offset, 0);

   int prev_x = mCursor->pos().x();

   mCursor->setPos(frame / mWaveForm->zoom(), 0);

   //TODO is there a better way?
   //invalidate a box around the last cursor position, so that it gets redrawn without the cursor
   QRectF rect(prev_x - 1, -100, 3, 200);
   mScene->invalidate(rect);
}

void WaveFormView::resizeEvent(QResizeEvent * event) {
   resetMatrix();
   //scale(((float)event->size().width() / 200.0), 1.0);
   //scale((float)geometry().width() / 200.0, 1.0);
   rotate(-90);
   //ensureVisible(mScene->sceneRect(), Qt::IgnoreAspectRatio);

   QGraphicsView::resizeEvent(event);
}

void WaveFormView::wheelEvent(QWheelEvent * event) {
   float amt = (float)event->delta() / 160.0;
   amt *= geometry().height();
   int frames = amt * mWaveForm->zoom();
   emit(seek_relative(frames));
}


void WaveFormView::mouseMoveEvent(QMouseEvent * event) {
   int diff = event->y() - mLastMousePos;
   mLastMousePos = event->y();
   int frames = mWaveForm->zoom() * diff;
   emit(seek_relative(frames));
}

void WaveFormView::mousePressEvent(QMouseEvent * event) {
   mLastMousePos = event->y();
   emit(mouse_down(true));
}

void WaveFormView::mouseReleaseEvent(QMouseEvent * event) {
   emit(mouse_down(false));
}
