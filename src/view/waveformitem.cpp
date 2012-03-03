#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QTransform>
#include <QGraphicsScene>
#include "waveformitem.hpp"
#include "audiobuffer.hpp"
#include <math.h>
#include <iostream>

using std::cout;
using std::endl;
using namespace DataJockey::View;

#define WIDTH 12 * 1024
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

WaveFormItem::WaveFormItem(QGraphicsItem * parent) : 
   QGraphicsItem(parent), mPen(), mBoundingRect(-1, -100, 1, 200), mZoom(10)
{
   setFlag(QGraphicsItem::ItemIsMovable, false);
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
   setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
   setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

WaveFormItem::~WaveFormItem(){
   mAudioBuffer.release();
}

QRectF WaveFormItem::boundingRect() const {
   return mBoundingRect;
}

void WaveFormItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
   if (!mAudioBuffer.valid())
      return;

   //cout << "exposed rect: " << option->exposedRect.left() << " " << option->exposedRect.right() << endl;

   double zoom = mZoom;
   const unsigned int bottom = MAX(0, option->exposedRect.left());
   const unsigned int top = option->exposedRect.right();//MIN(option->exposedRect.right(), floor((double)buf->length() / (double)zoom));
   const unsigned int length = mAudioBuffer->length();

   painter->setPen(mPen);
   for(unsigned int i = bottom; i < top; i++) {
      const unsigned int windowStart = (zoom * i);
      unsigned int windowEnd = MIN(length, windowStart + (double)zoom);
      if (windowEnd == windowStart && windowStart != length) {
         windowEnd += 1;
      }
      float value = 0;
      for (unsigned int j = windowStart; j < windowEnd; j++) {
         value = MAX(value, mAudioBuffer->sample(0, j));
         value = MAX(value, mAudioBuffer->sample(1, j));
      }
      painter->drawLine(i, -value * 100.0, i, value * 100.0);

      /*
      //value = 100.0 * bufRef()->raw_buffer()[0][i];
      if (i < length) {
      unsigned int value = 100.0 * data[i];
      painter->drawLine(i, -value, i, value);
      }
      //painter->drawLine(i, -100, i, 2 * value - 100);
      */
   }
}

QVariant WaveFormItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value) {
   //cout << "change: " << change << " value: " << value.toString().toStdString() << endl;
   return QGraphicsItem::itemChange(change, value);
}

void WaveFormItem::setPen(const QPen& pen) {
   mPen = pen;
}

void WaveFormItem::setAudioFile(const QString& fileName) {
   mAudioBuffer.reset(fileName);
   if (mAudioBuffer.valid()) {
      mBoundingRect.setWidth(mAudioBuffer->length() / (sizeof(float) * mZoom));
   } else {
      mBoundingRect.setWidth(0);
   }

   prepareGeometryChange();
}

void WaveFormItem::clearAudioFile(){
   mAudioBuffer.release();
   mBoundingRect.setWidth(0);
   prepareGeometryChange();
}

int WaveFormItem::audioFileFrames() {
   if (mAudioBuffer.valid())
      return mAudioBuffer->length();
   return 0;
}

void WaveFormItem::setZoom(int level){
   if (level > 1)
      mZoom = level;
   else
      mZoom = 1;

   if (mAudioBuffer.valid()) {
      mBoundingRect.setWidth(mAudioBuffer->length() / mZoom);
      prepareGeometryChange();
   }
}

int WaveFormItem::zoom() const {
   return mZoom;
}

