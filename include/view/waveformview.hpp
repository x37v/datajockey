#ifndef DATAJOCKEY_WAVEFORM_VIEW_HPP
#define DATAJOCKEY_WAVEFORM_VIEW_HPP

#include <QGraphicsView>
#include <QString>

class QGraphicsScene;
class QResizeEvent;
class QGraphicsLineItem;

namespace DataJockey {
   namespace View {
      class WaveFormItem;

      class WaveFormView : public QGraphicsView {
         public:
            WaveFormView(QWidget * parent = NULL);
            void set_audio_file(const QString& file_name);
            void set_audio_frame(int frame);
            virtual void resizeEvent(QResizeEvent * event);
         private:
            WaveFormItem * mWaveForm;
            QGraphicsScene * mScene;
            QGraphicsLineItem * mCursor;
      };
   }
}

#endif
