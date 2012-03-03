#ifndef DATAJOCKEY_WAVEFORM_VIEW_HPP
#define DATAJOCKEY_WAVEFORM_VIEW_HPP

#include <QGraphicsView>
#include <QString>

class QGraphicsScene;

namespace DataJockey {
   namespace View {
      class WaveFormItem;

      class WaveFormView : public QGraphicsView {
         public:
            WaveFormView(QWidget * parent = NULL);
            void set_audio_file(const QString& file_name);
            void set_audio_frame(int frame);
         private:
            WaveFormItem * mWaveForm;
            QGraphicsScene * mWaveFormScene;
      };
   }
}

#endif
