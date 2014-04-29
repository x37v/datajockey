#include "loopandjumpcontrolview.h"
#include "ui_loopandjumpcontrolview.h"
#include <QPushButton>

LoopAndJumpControlView::LoopAndJumpControlView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::LoopAndJumpControlView)
{
  ui->setupUi(this);

  for (int i = 0; i < 8; i++) {
    QPushButton * btn = new QPushButton(QString::number(i + 1), this);
    mJumpButtons.push_back(btn);
    ui->jumpButtonLayout->addWidget(btn);
  }

  QButtonGroup * loopGroup = new QButtonGroup(this);
  loopGroup->setExclusive(false);
  for (int i = 0; i < 4; i++) {
    QPushButton * btn = new QPushButton(QString::number(pow(2, i)), this);
    mLoopButtons.push_back(btn);
    ui->loopButtonLayout->addWidget(btn);

    btn->setCheckable(true);
    loopGroup->addButton(btn);
  }

#if 0
  //exclusive buttons but allowing none to be pressed
  QObject::connect(loopGroup,
      static_cast<void (QButtonGroup::*)(QAbstractButton *, bool)>(&QButtonGroup::buttonToggled),
      [&loopGroup](QAbstractButton * c, bool checked) {
      if (checked) {
        foreach (QAbstractButton * btn, loopGroup->buttons()) {
          if (btn != c)
            btn->setChecked(false);
        }
      }
  });
#endif
}

LoopAndJumpControlView::~LoopAndJumpControlView() {
  delete ui;
}

void LoopAndJumpControlView::updateEntry(dj::loop_and_jump_type_t type, int entry) {
}

void LoopAndJumpControlView::clearEntry(int entry) {
}

void LoopAndJumpControlView::clearAll() {
}

