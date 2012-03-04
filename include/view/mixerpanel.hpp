#ifndef DATAJOCKEY_MIXERPANEL_VIEW_HPP
#define DATAJOCKEY_MIXERPANEL_VIEW_HPP

#include <QWidget>
#include <QList>

class QSlider;

namespace DataJockey {
   namespace View {
      class Player;

      class MixerPanel : public QWidget {
         Q_OBJECT
         public:
            MixerPanel(QWidget * parent = NULL);
            virtual ~MixerPanel();
            QList<Player *> players() const;
            QSlider * cross_fade_slider() const;;
         private:
            QList<Player *> mPlayers;
            QSlider * mCrossFadeSlider;
      };
   }
}
#endif
