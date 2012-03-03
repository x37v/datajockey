#include "player_view.hpp"
#include "defines.hpp"
#include "waveformitem.hpp"

#include <QPushButton>
#include <QGridLayout>
#include <QSlider>
#include <QDial>
#include <QProgressBar>
#include <QGraphicsView>
#include <QGraphicsScene>

using namespace DataJockey::View;

struct button_info {
   int row;
   int col;
   bool checkable;
   QString label;
   QString name;
};

Player::Player(QWidget * parent, WaveformOrientation waveform_orientation) : QWidget(parent) {
   button_info items[] = {
      {0, 0, false, "ld", "load"},
      {0, 2, false, "rs", "reset"},
      {1, 0, true,  "cu", "cue"},
      {1, 1, true,  "sy", "sync"},
      {1, 2, true,  "pl", "play"},
      {2, 0, false, "<<", "seek_back"},
      {2, 2, false, ">>", "seek_forward"},
   };

   mTopLayout = new QBoxLayout(QBoxLayout::LeftToRight);
   if (waveform_orientation == WAVEFORM_LEFT)
      mTopLayout->setDirection(QBoxLayout::RightToLeft);

   mControlLayout = new QBoxLayout(QBoxLayout::TopToBottom);
   QGridLayout * button_layout = new QGridLayout();

   mTrackDescription = new QTextEdit(this);
   mTrackDescription->setReadOnly(true);
   mTrackDescription->setText("EMPTY");
   mControlLayout->addWidget(mTrackDescription);

   mProgressBar = new QProgressBar(this);
   mProgressBar->setTextVisible(true);
   mControlLayout->addWidget(mProgressBar);

   for (unsigned int i = 0; i < sizeof(items) / sizeof(button_info); i++) {
      QPushButton * btn = new QPushButton(items[i].label, this);
      btn->setCheckable(items[i].checkable);
      mButtons.insert(items[i].name, btn);
      button_layout->addWidget(btn, items[i].row, items[i].col);
   }
   mControlLayout->addLayout(button_layout);

   QString eq[3] = { "high", "mid", "low" };
   for (unsigned int i = 0; i < 3; i++) {
      QDial * dial = new QDial(this);
      dial->setRange(-one_scale, one_scale);
      mEqDials.insert(eq[i], dial);
      mControlLayout->addWidget(dial, 0, Qt::AlignHCenter);
   }

   mVolumeSlider = new QSlider(Qt::Vertical, this);
   mVolumeSlider->setRange(0, (int)(1.5 * (float)one_scale));
   mVolumeSlider->setValue(one_scale);
   mControlLayout->addWidget(mVolumeSlider, 1, Qt::AlignHCenter);

   mWaveFormView = new QGraphicsView(this);
   mWaveFormView->setVisible(waveform_orientation != WAVEFORM_NONE);
   mWaveFormView->rotate(-90);

   mWaveFormScene = new QGraphicsScene(mWaveFormView);

   mWaveForm = new WaveFormItem();
   mWaveFormScene->addItem(mWaveForm);

   mWaveFormView->setScene(mWaveFormScene);
   //mWaveFormView->fitInView(mWaveForm);
   //mWaveFormView->ensureVisible(mWaveFormScene->mWaveFormSceneRect(), Qt::IgnoreAspectRatio);

   mTopLayout->addLayout(mControlLayout);
   mTopLayout->addWidget(mWaveFormView);
   setLayout(mTopLayout);
}

QPushButton * Player::button(QString name) const { return mButtons[name]; }
QDial * Player::eq_dial(QString name) const { return mEqDials[name]; }
QSlider * Player::volume_slider() const { return mVolumeSlider; }
QProgressBar * Player::progress_bar() const { return mProgressBar; }

void Player::set_audio_file(const QString& file_name) {
   mWaveForm->setAudioFile(file_name);
   QRectF rect = mWaveForm->boundingRect();
   rect.setWidth(rect.width() + 2 * mWaveFormView->geometry().height());
   rect.setX(rect.x() - mWaveFormView->geometry().height());
   mWaveFormScene->setSceneRect(rect);
}

void Player::set_audio_frame(int frame) {
   int offset = mWaveFormView->geometry().height() / 4;
   mWaveFormView->centerOn(frame / mWaveForm->zoom() + offset, 0);
}