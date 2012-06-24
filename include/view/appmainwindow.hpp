#ifndef DATAJOCKEY_APPMAIN_WINDOW_HPP
#define DATAJOCKEY_APPMAIN_WINDOW_HPP

#include <QMainWindow>
#include "midimappingview.hpp"

class QContextMenuEvent;

class MainWindow : public QMainWindow {
   Q_OBJECT
   public:
      MainWindow(QWidget * parent = NULL);
      virtual ~MainWindow();
      dj::view::MIDIMapper * midi_mapper();

   protected:
      void contextMenuEvent(QContextMenuEvent *event);

   private slots:
      void about();
      void midi_mapping();

   private:
     void createActions();
     void createMenus();

     QAction *quitAct;
     QAction *aboutAct;
     QAction *midiMappingAct;

     QMenu *fileMenu;
     QMenu *preferencesMenu;
     QMenu *helpMenu;

     dj::view::MIDIMapper * mMIDIMapper;
};

#endif
