#ifndef DATAJOCKEY_AUDIOLEVEL_VIEW_HPP
#define DATAJOCKEY_AUDIOLEVEL_VIEW_HPP

#include <QWidget>
#include <QPen>
#include <QBrush>

namespace DataJockey {
   namespace View {
      class AudioLevel : public QWidget {
         Q_OBJECT
         public:
            AudioLevel(QWidget * parent = NULL);
            QSize minimumSizeHint() const;
            QSize sizeHint() const;

         public slots:
            void set_level(int percent);

         protected:
            void paintEvent(QPaintEvent *event);

         private:
            int mPercent;
            QPen mPen;
            QBrush mBrush;
      };
   }
}

#endif
