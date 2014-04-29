#ifndef LOOPANDJUMPCONTROLVIEW_H
#define LOOPANDJUMPCONTROLVIEW_H

#include <QWidget>
#include <QList>
#include "defines.hpp"

namespace Ui {
  class LoopAndJumpControlView;
}

class QPushButton;
class LoopAndJumpControlView : public QWidget {
  Q_OBJECT
  public:
    explicit LoopAndJumpControlView(QWidget *parent = 0);
    ~LoopAndJumpControlView();
  public slots:
    void updateEntry(dj::loop_and_jump_type_t type, int entry);
    void clearEntry(int entry);
    void clearAll();
  private:
    Ui::LoopAndJumpControlView *ui;
    QList<QPushButton *> mJumpButtons;
    QList<QPushButton *> mLoopButtons;
};

#endif // LOOPANDJUMPCONTROLVIEW_H
