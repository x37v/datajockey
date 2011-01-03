#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <QGraphicsItem>
#include <QRectF>
#include <QVariant>
#include <QPen>
#include <QString>
#include <QSharedMemory>

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
            void setAudioFile(const QString& fileName);
            void clearAudioFile();
            //zoom is #of frames per pixel, so 100 would mean we take the max of 100 frames per 1 unit in the horizontal
            void setZoom(int level);
            int zoom() const;
            unsigned int data_scale();
         protected:
            virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);
         private:
            QPen mPen;
            QRectF mBoundingRect;
            QSharedMemory mSharedBuffer;
            unsigned int mZoom;
            //our data doesn't actually have every frame, we have one data point for every mDataScale frames.
            unsigned int mDataScale;
      };
   }
}
#endif
