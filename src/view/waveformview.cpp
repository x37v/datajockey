#include "waveformview.hpp"
#include "waveformitem.hpp"

#include <QGraphicsScene>

using namespace DataJockey::View;

WaveFormView::WaveFormView(QWidget * parent) {
   rotate(-90);

   mWaveFormScene = new QGraphicsScene(this);

   mWaveForm = new WaveFormItem();
   mWaveFormScene->addItem(mWaveForm);

   this->setScene(mWaveFormScene);

   //mWaveFormView->fitInView(mWaveForm);
   //mWaveFormView->ensureVisible(mWaveFormScene->mWaveFormSceneRect(), Qt::IgnoreAspectRatio);
}

void WaveFormView::set_audio_file(const QString& file_name) {
   mWaveForm->setAudioFile(file_name);
   QRectF rect = mWaveForm->boundingRect();
   rect.setWidth(rect.width() + 2 * geometry().height());
   rect.setX(rect.x() - geometry().height());
   mWaveFormScene->setSceneRect(rect);
}

void WaveFormView::set_audio_frame(int frame) {
   int offset = geometry().height() / 4;
   centerOn(frame / mWaveForm->zoom() + offset, 0);
}
