#ifndef DATAJOCKEY_APPLICATION_HPP
#define DATAJOCKEY_APPLICATION_HPP

#include <QApplication>

class QWidget;

namespace DataJockey {
   namespace Audio { class AudioModel; }
   class Application : public QApplication {
      Q_OBJECT
      public:
         Application(int & argc, char ** argv);
      public slots:
         void pre_quit_actions();
      private:
         Audio::AudioModel * mAudioModel;
         QWidget * mTop;
   };
}

#endif
