#ifndef DATAJOCKEY_APPMAIN_WINDOW_HPP
#define DATAJOCKEY_APPMAIN_WINDOW_HPP

#include <QMainWindow>

class QContextMenuEvent;

class MainWindow : public QMainWindow {
   Q_OBJECT
   public:
      MainWindow(QWidget * parent = NULL);
      virtual ~MainWindow();
   protected:
      void contextMenuEvent(QContextMenuEvent *event);
      /*
   private slots:
      void preferences();
   private:
     void createActions();
     void createMenus();

     QMenu *preferencesMenu;
     */
};

#endif
