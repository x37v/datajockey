#ifndef DJ_MIDIMAPPING_VIEW_HPP
#define DJ_MIDIMAPPING_VIEW_HPP

#include <QWidget>

class QTableWidget;

namespace dj {
   namespace view {
      class MIDIMapper : public QWidget {
         Q_OBJECT
         public:
            MIDIMapper(QWidget * parent = NULL, Qt::WindowFlags f = 0);
            ~MIDIMapper();
         private:
            QTableWidget * mPlayerTable;
      };
   }
}

#endif

