#ifndef RENAMEABLE_TAB_WIDGET_H
#define RENAMEABLE_TAB_WIDGET_H

#include <QTabBar>
#include <QTabWidget>

class RenameableTabWidget : public QTabWidget {
  Q_OBJECT
  public:
    RenameableTabWidget(QWidget * parent = NULL);
};

class RenameableTabBar : public QTabBar {
  Q_OBJECT
  public:
    RenameableTabBar(QWidget * parent = NULL);
  protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *e);
};

#endif
