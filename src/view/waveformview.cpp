#include "waveformview.hpp"
#include "waveformitem.hpp"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QGraphicsLineItem>

using namespace DataJockey::View;

WaveFormView::WaveFormView(QWidget * parent) {
   rotate(-90);

   mScene = new QGraphicsScene(this);

   mWaveForm = new WaveFormItem();
   mWaveForm->setZoom(150);
   mScene->addItem(mWaveForm);

   mCursor = new QGraphicsLineItem(0, -100, 0, 100);
   mCursor->setPen(QPen(Qt::green));
   mScene->addItem(mCursor);

   this->setScene(mScene);

   //mWaveFormView->fitInView(mWaveForm);
   //mWaveFormView->ensureVisible(mScene->mSceneRect(), Qt::IgnoreAspectRatio);
}

void WaveFormView::set_audio_file(const QString& file_name) {
   mWaveForm->setAudioFile(file_name);
   QRectF rect = mWaveForm->boundingRect();
   rect.setWidth(rect.width() + 2 * geometry().height());
   rect.setX(rect.x() - geometry().height());
   mScene->setSceneRect(rect);
}

void WaveFormView::set_audio_frame(int frame) {
   int offset = geometry().height() / 4;
   centerOn(frame / mWaveForm->zoom() + offset, 0);
   mCursor->setPos(frame / mWaveForm->zoom(), 0);
}

void WaveFormView::resizeEvent(QResizeEvent * event) {
   //resetMatrix();
   //scale(((float)event->size().width() / 200.0), 1.0);
   //rotate(-90);

   QGraphicsView::resizeEvent(event);
}

