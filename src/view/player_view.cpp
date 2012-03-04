#include "player_view.hpp"
#include "defines.hpp"
#include "waveformitem.hpp"
#include "audiobufferreference.hpp"
#include "audiobuffer.hpp"

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

Player::Player(QWidget * parent, WaveformOrientation waveform_orientation) : QWidget(parent), mFrames(0) {
   button_info items[] = {
      {0, 0, false, "ld", "load"},
      {0, 2, false, "rs", "reset"},
      {1, 0, true,  "cu", "cue"},
      {1, 1, true,  "sy", "sync"},
      {1, 2, true,  "||", "pause"},
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
      //XXX can we style out text? QPushButton * btn = new QPushButton(items[i].label, this);
      QPushButton * btn = new QPushButton(this);
      btn->setProperty("dj_name", items[i].name);
      btn->setCheckable(items[i].checkable);
      mButtons.insert(items[i].name, btn);
      button_layout->addWidget(btn, items[i].row, items[i].col);
   }
   mControlLayout->addLayout(button_layout);

   QString eq[3] = { "high", "mid", "low" };
   for (unsigned int i = 0; i < 3; i++) {
      QDial * dial = new QDial(this);
      dial->setRange(-one_scale, one_scale);
      dial->setProperty("dj_name", QString("dj_eq_") + eq[i]);
      mEqDials.insert(eq[i], dial);
      mControlLayout->addWidget(dial, 0, Qt::AlignHCenter);
   }

   mVolumeSlider = new QSlider(Qt::Vertical, this);
   mVolumeSlider->setRange(0, (int)(1.5 * (float)one_scale));
   mVolumeSlider->setValue(one_scale);
   mControlLayout->addWidget(mVolumeSlider, 1, Qt::AlignHCenter);

   mWaveFormView = new WaveFormView(this);
   mWaveFormView->setVisible(waveform_orientation != WAVEFORM_NONE);

   mTopLayout->addLayout(mControlLayout);
   mTopLayout->addWidget(mWaveFormView);
   setLayout(mTopLayout);

   QObject::connect(mWaveFormView,
         SIGNAL(seek_relative(int)),
         this,
         SLOT(relay_seek_relative(int)));
   QObject::connect(mWaveFormView,
         SIGNAL(mouse_down(bool)),
         this,
         SLOT(relay_mouse_button(bool)));
}

QPushButton * Player::button(QString name) const { return mButtons[name]; }
QList<QPushButton *> Player::buttons() const { return mButtons.values(); }
QDial * Player::eq_dial(QString name) const { return mEqDials[name]; }
QList<QDial *> Player::eq_dials() const { return mEqDials.values(); }
QSlider * Player::volume_slider() const { return mVolumeSlider; }
QProgressBar * Player::progress_bar() const { return mProgressBar; }

void Player::set_audio_file(const QString& file_name) {
   Audio::AudioBufferReference ref(file_name);

   mWaveFormView->set_audio_file(file_name);
   if (ref.valid())
      mFrames = ref->length();
   else
      mFrames = 0;
   mProgressBar->setValue(0);
}

void Player::set_audio_frame(int frame) {
   mWaveFormView->set_audio_frame(frame);
   if (mFrames) {
      mProgressBar->setValue(frame / (mFrames / 100));
   } else
      mProgressBar->setValue(0);
}

void Player::relay_seek_relative(int frames) { emit(seek_relative(frames)); }
void Player::relay_mouse_button(bool down) { emit(seeking(down)); }

