#include "player_view.hpp"
#include "defines.hpp"

#include <QPushButton>
#include <QGridLayout>
#include <QSlider>
#include <QDial>
#include <QProgressBar>

using namespace DataJockey::View;

struct button_info {
   int row;
   int col;
   bool checkable;
   QString label;
   QString name;
};

Player::Player(QWidget * parent) : QWidget(parent) {
   button_info items[] = {
      {0, 0, false, "ld", "load"},
      {0, 2, false, "rs", "reset"},
      {1, 0, true,  "cu", "cue"},
      {1, 1, true,  "sy", "sync"},
      {1, 2, true,  "pl", "play"},
      {2, 0, false, "<<", "seek_back"},
      {2, 2, false, ">>", "seek_forward"},
   };

   mTopLayout = new QBoxLayout(QBoxLayout::TopToBottom, this);
   QGridLayout * button_layout = new QGridLayout();

   mTrackDescription = new QTextEdit(this);
   mTrackDescription->setReadOnly(true);
   mTrackDescription->setText("EMPTY");
   mTopLayout->addWidget(mTrackDescription);

   mProgressBar = new QProgressBar(this);
   mProgressBar->setTextVisible(true);
   mProgressBar->setValue(50);
   mTopLayout->addWidget(mProgressBar);

   for (unsigned int i = 0; i < sizeof(items) / sizeof(button_info); i++) {
      QPushButton * btn = new QPushButton(items[i].label, this);
      btn->setCheckable(items[i].checkable);
      mButtons.insert(items[i].name, btn);
      button_layout->addWidget(btn, items[i].row, items[i].col);
   }
   mTopLayout->addLayout(button_layout);

   QString eq[3] = {
      "high",
      "mid",
      "low"
   };
   for (unsigned int i = 0; i < 3; i++) {
      QDial * dial = new QDial(this);
      dial->setRange(-one_scale, one_scale);
      mEqDials.insert(eq[i], dial);
      mTopLayout->addWidget(dial, 0, Qt::AlignHCenter);
   }

   mVolumeSlider = new QSlider(Qt::Vertical, this);
   mVolumeSlider->setRange(0, (int)(1.5 * (float)one_scale));
   mVolumeSlider->setValue(one_scale);
   mTopLayout->addWidget(mVolumeSlider, 1, Qt::AlignHCenter);

   setLayout(mTopLayout);
}

QPushButton * Player::button(QString name) const { return mButtons[name]; }
QDial * Player::eq_dial(QString name) const { return mEqDials[name]; }
QSlider * Player::volume_slider() const { return mVolumeSlider; }
QProgressBar * Player::progress_bar() const { return mProgressBar; }
