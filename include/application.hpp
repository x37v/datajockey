#ifndef DATAJOCKEY_APPLICATION_HPP
#define DATAJOCKEY_APPLICATION_HPP

#include <QApplication>
#include <QProcess>
#include "mixerpanel.hpp"

class QWidget;
class MainWindow;

namespace dj {
  namespace audio { class AudioModel; }
  namespace controller { 
    class MIDIMapper;
    class OSCSender;
    class HistoryManager;
  }

  class Application : public QApplication {
    Q_OBJECT
    public:
      Application(int & argc, char ** argv);
    public slots:
      void post_start_actions();
      void pre_quit_actions();
      void select_work(int work_id);
      void player_trigger(int player_index, QString name);
    protected slots:
      void startup_script_error(QProcess::ProcessError error);
    private:
      audio::AudioModel * mAudioModel;
      view::MixerPanel * mMixerPanel;
      controller::MIDIMapper * mMIDIMapper;
      controller::OSCSender * mOSCSender;
      controller::HistoryManager * mHistoryManger;
      MainWindow * mTop;
      int mCurrentwork;
  };
}

#endif
