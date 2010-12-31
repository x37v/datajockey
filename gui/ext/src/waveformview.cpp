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

#define WIDTH (20 * 44100)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

DataJockey::View::WaveFormView::WaveFormView(QGraphicsItem * parent) : 
   QGraphicsItem(parent)
{
   setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
   setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

DataJockey::View::WaveFormView::~WaveFormView(){
}

QRectF DataJockey::View::WaveFormView::boundingRect() const {
   return QRectF(0, -100, WIDTH, 200);
}

void DataJockey::View::WaveFormView::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget) {
   painter->drawLine(0, 0, 1024, 0);
   qreal zoom = option->levelOfDetailFromTransform(painter->worldTransform());
   /*
   cout << "zoom: " << zoom << endl;

   cout << "exposed rect: " << option->exposedRect.left() << " " << option->exposedRect.right() << endl;
   */

   unsigned int bottom = MAX(0, option->exposedRect.left());
   unsigned int top = MIN(option->exposedRect.right(), WIDTH);

   for(unsigned int i = bottom; i < top; i++) {
      int x = -100.0 * sin((3.14f * 2.0 * i) / 44100.0);
      painter->drawLine(i, 0, i, x);
   }
}

QVariant DataJockey::View::WaveFormView::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value) {
   //cout << "change: " << change << " value: " << value.toString().toStdString() << endl;
   return QGraphicsItem::itemChange(change, value);
}
