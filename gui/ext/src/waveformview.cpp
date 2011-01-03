#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QTransform>
#include <QGraphicsScene>
#include "waveformview.hpp"
#include <math.h>
#include <iostream>

//our data is already zoomed in, so this is the minimum zoom value
#define DATASCALE_DEFAULT 32

using std::cout;
using std::endl;
using namespace DataJockey::View;

#define WIDTH 12 * 1024
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

WaveFormView::WaveFormView(QGraphicsItem * parent) : 
   QGraphicsItem(parent), mPen(), mBoundingRect(-1, -100, 1, 200), mZoom(10), mDataScale(DATASCALE_DEFAULT)
{
   setFlag(QGraphicsItem::ItemIsMovable, false);
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
   setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
   setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

WaveFormView::~WaveFormView(){
   if (mSharedBuffer.isAttached())
      mSharedBuffer.detach();
}

QRectF WaveFormView::boundingRect() const {
   return mBoundingRect;
}

void WaveFormView::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
   if (!mSharedBuffer.isAttached())
      return;

   //cout << "exposed rect: " << option->exposedRect.left() << " " << option->exposedRect.right() << endl;

   mSharedBuffer.lock();
   float * data = (float *)mSharedBuffer.constData();
   if (data) {
      double zoom = mZoom;
      const unsigned int bottom = MAX(0, option->exposedRect.left());
      const unsigned int top = option->exposedRect.right();//MIN(option->exposedRect.right(), floor((double)buf->length() / (double)zoom));
      const unsigned int length = mSharedBuffer.size() / sizeof(float);

      painter->setPen(mPen);
      for(unsigned int i = bottom; i < top; i++) {
         const unsigned int windowStart = (zoom * i) / (double)mDataScale;
         unsigned int windowEnd = MIN(length, windowStart + ((double)zoom / mDataScale));
         if (windowEnd == windowStart && windowStart != length) {
            windowEnd += 1;
         }
         unsigned int value = 0;
         for (unsigned int j = windowStart; j < windowEnd; j++) {
            value = MAX(value, 100.0 * data[j]);
         }
         painter->drawLine(i, -value, i, value);

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
   mSharedBuffer.unlock();
}

QVariant WaveFormView::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value) {
   //cout << "change: " << change << " value: " << value.toString().toStdString() << endl;
   return QGraphicsItem::itemChange(change, value);
}

void WaveFormView::setPen(const QPen& pen) {
   mPen = pen;
}

void WaveFormView::setAudioFile(const QString& fileName) {
   if (mSharedBuffer.isAttached())
      mSharedBuffer.detach();

   mSharedBuffer.setKey(fileName);
   if (mSharedBuffer.attach(QSharedMemory::ReadOnly)) {
      mBoundingRect.setWidth(mSharedBuffer.size() * mDataScale / (sizeof(float) * mZoom));
   } else {
      cout << "cannot attach" << endl;
      mBoundingRect.setWidth(0);
   }

   prepareGeometryChange();
}

void WaveFormView::clearAudioFile(){
   if (mSharedBuffer.isAttached())
      mSharedBuffer.detach();
   mBoundingRect.setWidth(0);
   prepareGeometryChange();
}

void WaveFormView::setZoom(int level){
   if (level > 1)
      mZoom = level;
   else
      mZoom = 1;

   if (mSharedBuffer.isAttached()) {
      mBoundingRect.setWidth(mSharedBuffer.size() * mDataScale / (sizeof(float) * mZoom));
      prepareGeometryChange();
   }
}

int WaveFormView::zoom() const {
   return mZoom;
}

unsigned int WaveFormView::data_scale() {
   return mDataScale;
}

