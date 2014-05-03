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
    connect(btn, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), [this, i](bool /*checked*/) {
        emit(buttonTriggered(i));
    });
    btn->setProperty("jump_type", "none");
    btn->setStyleSheet("QPushButton[jump_type=jump] { background-color: yellow; color: black; }");
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
  if (entry < 0 || entry >= mJumpButtons.size())
    return;
  QPushButton * btn = mJumpButtons[entry];
  switch (type) {
    case dj::LOOP:
      btn->setProperty("jump_type", "loop");
      break;
    case dj::JUMP:
      btn->setProperty("jump_type", "jump");
      break;
    default:
      break;
  }

  //apply the styling
  //http://qt-project.org/wiki/DynamicPropertiesAndStylesheets
  btn->style()->unpolish(btn);
  btn->style()->polish(btn);
  btn->update();
}

#include <iostream>
using namespace std;

void LoopAndJumpControlView::clearEntry(int entry) {
  if (entry < 0 || entry >= mJumpButtons.size())
    return;
  QPushButton * btn = mJumpButtons[entry];
  btn->setProperty("jump_type", "none");
  btn->style()->unpolish(btn);
  btn->style()->polish(btn);
  btn->update();
  cout << "clear: " << entry << endl;
}

void LoopAndJumpControlView::clearAll() {
  for (int i = 0; i < mJumpButtons.size(); i++)
    clearEntry(i);
}

