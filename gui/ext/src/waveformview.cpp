#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QTransform>
#include <QGraphicsScene>
#include "waveformview.hpp"
#include <math.h>
#include <iostream>
#include "audiobuffer.hpp"
#include "audiocontroller.hpp"


using std::cout;
using std::endl;
using namespace DataJockey::View;

#define WIDTH 12 * 1024
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

WaveFormView::WaveFormView(QGraphicsItem * parent) : 
   QGraphicsItem(parent), mPen(), mAudioBufferReference(), mBoundingRect(0, -100, 1, 200)
{
   setFlag(QGraphicsItem::ItemIsMovable, false);
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
   setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
   setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

WaveFormView::~WaveFormView(){
}

QRectF WaveFormView::boundingRect() const {
   return mBoundingRect;
}

void WaveFormView::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
   //keep a local copy of the reference just in case it is swapped out while we're drawing
   Audio::AudioBufferReference bufRef = mAudioBufferReference;

   if (!bufRef.valid())
      return;

   //cout << "exposed rect: " << option->exposedRect.left() << " " << option->exposedRect.right() << endl;

   unsigned int bottom = MAX(0, option->exposedRect.left());
   unsigned int top = MIN(option->exposedRect.right(), bufRef()->length());

   painter->setPen(mPen);
   for(unsigned int i = bottom; i < top; i++) {
      unsigned int sample = 100.0 * bufRef()->raw_buffer()[0][i];
      painter->drawLine(i, -sample, i, sample);
   }
}

QVariant WaveFormView::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value) {
   //cout << "change: " << change << " value: " << value.toString().toStdString() << endl;
   return QGraphicsItem::itemChange(change, value);
}

void WaveFormView::setPen(const QPen& pen) {
   mPen = pen;
}

void WaveFormView::setAudioFile(const QString& fileName) {
   mAudioBufferReference.reset(fileName);
   if (mAudioBufferReference.valid())
      mBoundingRect.setWidth(mAudioBufferReference()->length());
   else
      mBoundingRect.setWidth(0);
   prepareGeometryChange();
}

void WaveFormView::clearAudioFile(){
   mAudioBufferReference.release();
   mBoundingRect.setWidth(0);
   prepareGeometryChange();
}
