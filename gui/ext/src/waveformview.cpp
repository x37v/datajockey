#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QTransform>
#include <QGraphicsScene>
#include "waveformview.hpp"
#include <math.h>
#include <iostream>

using std::cout;
using std::endl;
using namespace DataJockey::View;

#define WIDTH (20 * 44100)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

WaveFormView::WaveFormView(QGraphicsItem * parent) : 
   QGraphicsItem(parent), mPen()
{
   setFlag(QGraphicsItem::ItemIsMovable, false);
   setFlag(QGraphicsItem::ItemIsSelectable, false);
   setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
   setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

WaveFormView::~WaveFormView(){
}

QRectF WaveFormView::boundingRect() const {
   return QRectF(0, -100, WIDTH, 200);
}

void WaveFormView::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
   painter->drawLine(0, 0, 1024, 0);
   /*
   qreal zoom = option->levelOfDetailFromTransform(painter->worldTransform());
   cout << "zoom: " << zoom << endl;
   */

//   cout << "exposed rect: " << option->exposedRect.left() << " " << option->exposedRect.right() << endl;

   unsigned int bottom = MAX(0, option->exposedRect.left());
   unsigned int top = MIN(option->exposedRect.right(), WIDTH);

   painter->setPen(mPen);
   for(unsigned int i = bottom; i < top; i++) {
      int x = -100.0 * sin((3.14f * 2.0 * i) / 44100.0);
      painter->drawLine(i, 0, i, x);
   }
}

QVariant WaveFormView::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value) {
   //cout << "change: " << change << " value: " << value.toString().toStdString() << endl;
   return QGraphicsItem::itemChange(change, value);
}

void WaveFormView::setPen(const QPen& pen) {
   mPen = pen;
}
