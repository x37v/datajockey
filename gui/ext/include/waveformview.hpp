#ifndef WAVEFORM_VIEW_H
#define WAVEFORM_VIEW_H

#include <QGraphicsItem>
#include <QRectF>
#include <QVariant>
#include <QPen>
#include <QString>
#include "audiobufferreference.hpp"

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
         protected:
            virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant & value);
         private:
            QPen mPen;
            QRectF mBoundingRect;
            Audio::AudioBufferReference mAudioBufferReference;
      };
   }
}
#endif
