#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <QGraphicsItem>
#include <QRectF>
#include <QVariant>
#include <QPen>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace DataJockey {
   namespace View {
      class WaveFormView : public QGraphicsItem {
         public:
            WaveFormView(QGraphicsItem * parent = NULL);
            virtual ~WaveFormView();
            virtual QRectF boundingRect() const;
            virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
            void setPen(const QPen& pen);
         protected:
            virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);
         private:
            QPen mPen;

      };
   }
}
#endif
