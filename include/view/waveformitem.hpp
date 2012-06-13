#ifndef WAVEFORM_ITEM_H
#define WAVEFORM_ITEM_H

#include <QGraphicsItem>
#include <QRectF>
#include <QVariant>
#include <QPen>
#include <QString>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace dj {
   namespace view {
      class WaveFormItem : public QGraphicsItem {
         public:
            WaveFormItem(QGraphicsItem * parent = NULL);
            virtual ~WaveFormItem();
            virtual QRectF boundingRect() const;
            virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0);
            void setPen(const QPen& pen);
            void setAudioFile(const QString& fileName);
            void clearAudioFile();
            int audioFileFrames();
            //zoom is #of frames per pixel, so 100 would mean we take the max of 100 frames per 1 unit in the horizontal
            void setZoom(int level);
            int zoom() const;
         protected:
            virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);
         private:
            QPen mPen;
            QRectF mBoundingRect;
            audio::AudioBufferPtr mAudioBuffer;
            unsigned int mZoom;
      };
   }
}
#endif
