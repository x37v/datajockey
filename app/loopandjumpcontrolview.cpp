#include "loopandjumpcontrolview.h"
#include "ui_loopandjumpcontrolview.h"
#include <QPushButton>

LoopAndJumpControlView::LoopAndJumpControlView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::LoopAndJumpControlView)
{
  ui->setupUi(this);

  ui->deleteJumpButton->setCheckable(true);
  connect(ui->deleteJumpButton, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), [this](bool checked) {
      if (checked)
        emit(triggered("jump_clear_next"));
      else
        emit(triggered("jump_clear_next_abort"));
  });

  for (int i = 0; i < 8; i++) {
    QPushButton * btn = new QPushButton(QString::number(i + 1), this);
    mJumpButtons.push_back(btn);
    ui->jumpButtonLayout->addWidget(btn);
    connect(btn, static_cast<void (QPushButton::*)(bool)>(&QPushButton::clicked), [this, i](bool /*checked*/) {
        emit(valueChangedInt("jump", i));
    });
    btn->setProperty("jump_type", "none");
  }

  for (int i = -2; i < 5; i++) {
    QPushButton * l = new QPushButton(QString::number(pow(2, i)), this);
    mLoopButtons.push_back(l);
    ui->loopButtonLayout->addWidget(l);
    l->setCheckable(true);

    //exclusive buttons but allowing none to be pressed
    QObject::connect(l, &QPushButton::toggled, [this, l, i](bool down) {
      if (down) {
        emit(valueChangedInt("loop_length", i));
        for (QPushButton * b: mLoopButtons) {
          if (b != l && b->isChecked())
            b->setChecked(false);
        }
      } else {
        bool all_off = true;
        for (QPushButton * b: mLoopButtons) {
          if (b->isChecked()) {
            all_off = false;
            break;
          }
        }
        if (all_off)
          emit(triggered("loop_off"));
      }
    });
  }

}

LoopAndJumpControlView::~LoopAndJumpControlView() {
  delete ui;
}

void LoopAndJumpControlView::updateEntry(dj::loop_and_jump_type_t type, int entry) {
  if (entry < 0 || entry >= mJumpButtons.size())
    return;
  QPushButton * btn = mJumpButtons[entry];
  switch (type) {
    case dj::LOOP_BEAT:
    case dj::LOOP_FRAME:
      btn->setProperty("jump_type", "loop");
      break;
    case dj::JUMP_BEAT:
    case dj::JUMP_FRAME:
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

void LoopAndJumpControlView::clearEntry(int entry) {
  if (entry < 0 || entry >= mJumpButtons.size())
    return;
  QPushButton * btn = mJumpButtons[entry];
  btn->setProperty("jump_type", "none");
  btn->style()->unpolish(btn);
  btn->style()->polish(btn);
  btn->update();

  ui->deleteJumpButton->setChecked(false);
}

void LoopAndJumpControlView::clearAll() {
  for (int i = 0; i < mJumpButtons.size(); i++)
    clearEntry(i);
}

