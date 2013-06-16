#include "playerview.hpp"
#include "defines.hpp"
#include "waveformviewgl.hpp"
#include "audiobuffer.hpp"
#include "audiolevel.hpp"

#include <QPushButton>
#include <QGridLayout>
#include <QSlider>
#include <QDial>
#include <QProgressBar>
#include <QTimer>
#include <QSpinBox>

using namespace dj::view;


struct button_info {
   int row;
   int col;
   bool checkable;
   QString label;
   QString name;
   bool auto_repeat;
};

SpeedSpinBox::SpeedSpinBox(QWidget * parent) : QSpinBox(parent) {
   mValidator = new QDoubleValidator(this);
   setRange(-dj::one_scale, dj::one_scale);
   mValidator->setRange(-100.0, 100.0, 1);
}

QString SpeedSpinBox::textFromValue(int value) const {
   double dvalue = value * 100;
   dvalue /= dj::one_scale;
   return QString::number(dvalue, 'f', 1);
}

int SpeedSpinBox::valueFromText(const QString & text) const {
   double dvalue = text.toDouble();
   return (dvalue / 100.0) * dj::one_scale;
}

QValidator::State SpeedSpinBox::validate(QString & text, int & pos) const {
   return mValidator->validate(text, pos);
}

Player::Player(QWidget * parent, WaveformOrientation waveform_orientation) : QWidget(parent), mFrames(0) {
   button_info items[] = {
      {0, 0, false, "ld", "load", false},
      {0, 2, false, "rs", "reset", false},
      {1, 0, true,  "cu", "cue", false},
      {1, 1, true,  "sy", "sync", false},
      {1, 2, true,  "||", "pause", false},
      {2, 0, false, "<<", "seek_back", true},
      {2, 2, false, ">>", "seek_forward", true},
      {3, 0, false, "<b", "bump_back", true},
      {3, 2, false, "b>", "bump_forward", true},
      {4, 0, false, "<l", "loop_shift_back", true},
      {4, 1, true,  "lp",  "loop_now", false},
      {4, 2, false, "l>", "loop_shift_forward", true},
   };

   mTopLayout = new QBoxLayout(QBoxLayout::LeftToRight);
   if (waveform_orientation == WAVEFORM_LEFT)
      mTopLayout->setDirection(QBoxLayout::RightToLeft);

   mControlLayout = new QBoxLayout(QBoxLayout::TopToBottom);
   QGridLayout * button_layout = new QGridLayout();

   for (unsigned int i = 0; i < 2; i++) {
      mTrackDescription[i] = new QLineEdit(this);
      mTrackDescription[i]->setText("");
      mTrackDescription[i]->setReadOnly(true);
      mControlLayout->addWidget(mTrackDescription[i]);
   }

   mProgressBar = new QProgressBar(this);
   mProgressBar->setTextVisible(true);
   mControlLayout->addWidget(mProgressBar);

   for (unsigned int i = 0; i < sizeof(items) / sizeof(button_info); i++) {
      //XXX can we style out text? QPushButton * btn = new QPushButton(items[i].label, this);
      QPushButton * btn = new QPushButton(this);
      btn->setProperty("dj_name", items[i].name);
      btn->setCheckable(items[i].checkable);
      mButtons.insert(items[i].name, btn);
      button_layout->addWidget(btn, items[i].row, items[i].col + 1);
      //XXX tmp
      if (items[i].name.contains("bump_") || items[i].name.contains("loop"))
         btn->setText(items[i].label);
      if (items[i].auto_repeat)
         btn->setAutoRepeat(true);
   }
   //sync is checked to start out with
   mButtons["sync"]->setChecked(true);

   button_layout->setSpacing(0);
   button_layout->setColumnStretch(0, 100);
   button_layout->setColumnStretch(4, 100);
   mControlLayout->addLayout(button_layout);

   mLoopMeasures = new QSpinBox(this);
   mLoopMeasures->setRange(1, 16);
   QLabel * loop_lab = new QLabel("loop measures", this);
   mControlLayout->addWidget(loop_lab, 0, Qt::AlignHCenter);
   mControlLayout->addWidget(mLoopMeasures, 0, Qt::AlignHCenter);

   mSpeedView = new SpeedSpinBox(this);
   mSpeedView->setDisabled(true);
   QLabel * speed_lab = new QLabel("speed %", this);
   mControlLayout->addWidget(speed_lab, 0, Qt::AlignHCenter);
   mControlLayout->addWidget(mSpeedView, 0, Qt::AlignHCenter);

   QString eq[3] = { "high", "mid", "low" };
   for (unsigned int i = 0; i < 3; i++) {
      QDial * dial = new QDial(this);
      dial->setNotchesVisible(true);
      dial->setFixedSize(dial->minimumSizeHint());
      dial->setSingleStep(one_scale / 64);
      dial->setPageStep(one_scale / 16);
      dial->setRange(-one_scale, one_scale);
      dial->setProperty("dj_name", QString("eq_") + eq[i]);
      mEqDials.insert(eq[i], dial);
      mControlLayout->addWidget(dial, 0, Qt::AlignHCenter);
   }

   mVolumeSlider = new QSlider(Qt::Vertical, this);
   mVolumeSlider->setRange(0, static_cast<int>(1.5 * static_cast<float>(one_scale)));
   mVolumeSlider->setValue(one_scale);
   mVolumeSlider->setMinimumHeight(dj::volume_slider_height);
   mVolumeSlider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

   mAudioLevelView = new AudioLevel(this);

   mSliderLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight);
   mSliderLevelLayout->addStretch(100);
   mSliderLevelLayout->addWidget(mVolumeSlider, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addWidget(mAudioLevelView, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addStretch(100);
   mSliderLevelLayout->setContentsMargins(0,0,0,0);

   mControlLayout->addStretch(100);
   mControlLayout->addLayout(mSliderLevelLayout);
   mControlLayout->setContentsMargins(0,0,0,0);

   mWaveFormZoomedView = new WaveFormViewGL(this, true);
   mWaveFormZoomedView->setVisible(waveform_orientation != WAVEFORM_NONE);
   mWaveFormZoomedView->setMinimumWidth(50);

   mWaveFormFullView = new WaveFormViewGL(this, true, true);
   mWaveFormFullView->setVisible(waveform_orientation != WAVEFORM_NONE);
   mWaveFormFullView->setMinimumWidth(20);

   mTopLayout->addLayout(mControlLayout, 0);
   mTopLayout->addWidget(mWaveFormZoomedView, 10);
   mTopLayout->addWidget(mWaveFormFullView, 4);
   mTopLayout->setContentsMargins(0,0,0,0);
   setLayout(mTopLayout);

   QObject::connect(mWaveFormZoomedView, SIGNAL(seek_relative(int)),
         SIGNAL(seek_frame_relative(int)));
   QObject::connect(mWaveFormZoomedView, SIGNAL(mouse_down(bool)),
         SIGNAL(seeking(bool)));
   QObject::connect(mWaveFormFullView, SIGNAL(frame_clicked(int)),
         SIGNAL(seek_to_frame(int)));

   QObject::connect(mButtons["sync"], SIGNAL(toggled(bool)),
         mSpeedView, SLOT(setDisabled(bool)));
   QObject::connect(mButtons["sync"], SIGNAL(toggled(bool)),
         mButtons["bump_forward"], SLOT(setDisabled(bool)));
   QObject::connect(mButtons["sync"], SIGNAL(toggled(bool)),
         mButtons["bump_back"], SLOT(setDisabled(bool)));
   mButtons["bump_forward"]->setDisabled(true);
   mButtons["bump_back"]->setDisabled(true);
}

QPushButton * Player::button(QString name) const { return mButtons[name]; }
QList<QPushButton *> Player::buttons() const { return mButtons.values(); }
QSpinBox * Player::loop_measures_control() const { return mLoopMeasures; }
QDial * Player::eq_dial(QString name) const { return mEqDials[name]; }
QList<QDial *> Player::eq_dials() const { return mEqDials.values(); }
QSlider * Player::volume_slider() const { return mVolumeSlider; }
QSpinBox * Player::speed_view() const { return mSpeedView; }
QProgressBar * Player::progress_bar() const { return mProgressBar; }
QRect Player::slider_level_geometry() const { return mSliderLevelLayout->geometry(); }

void Player::set_audio_level(int percent) { mAudioLevelView->set_level(percent); }

void Player::set_buffers(audio::AudioBufferPtr audio_buffer, audio::BeatBufferPtr beat_buffer) {
   mWaveFormZoomedView->set_buffers(audio_buffer, beat_buffer);
   mWaveFormFullView->set_buffers(audio_buffer, beat_buffer);
   mProgressBar->setValue(0);
   if (audio_buffer)
      mFrames = audio_buffer->length();
   else
      mFrames = 0;
}

void Player::set_audio_frame(int frame) {
   mWaveFormZoomedView->set_frame(frame);
   mWaveFormFullView->set_frame(frame);

   if (mFrames) {
      mProgressBar->setValue(frame / (mFrames / 100));
   } else
      mProgressBar->setValue(0);
}

void Player::set_song_description(QString description) {
   QStringList des = description.split("\n");
   if (des.isEmpty() || description.isEmpty()) {
      mTrackDescription[0]->setText("");
      mTrackDescription[1]->setText("");
   } else {
      mTrackDescription[0]->setText(des[0]);
      mTrackDescription[1]->setText(description.remove(0, des[0].length() + 1));
   }
   mTrackDescription[0]->setCursorPosition(0);
   mTrackDescription[1]->setCursorPosition(0);
}

void Player::set_cuepoint(int id, int frame) {
  //XXX make color configurable
  mWaveFormFullView->add_marker(id, frame, Qt::cyan);
  mWaveFormZoomedView->add_marker(id, frame, Qt::cyan);
}

void Player::loop_start(int id, int frame) {
  QHash<int, loop_t>::iterator it = mLoops.find(id);
  loop_t loop;
  if (it != mLoops.end())
    loop = it.value();
  loop.start_frame = frame;

  mLoops[id] = loop;
  if (loop.enabled) {
    mWaveFormFullView->add_loop(id, loop.start_frame, loop.end_frame);
    mWaveFormZoomedView->add_loop(id, loop.start_frame, loop.end_frame);
  }
}

void Player::loop_end(int id, int frame) {
  QHash<int, loop_t>::iterator it = mLoops.find(id);
  loop_t loop;
  if (it != mLoops.end())
    loop = it.value();
  loop.end_frame = frame;

  mLoops[id] = loop;
  if (loop.enabled) {
    mWaveFormFullView->add_loop(id, loop.start_frame, loop.end_frame);
    mWaveFormZoomedView->add_loop(id, loop.start_frame, loop.end_frame);
  }
}

void Player::loop_enable(int id, bool enable) {
  QHash<int, loop_t>::iterator it = mLoops.find(id);
  loop_t loop;
  if (it != mLoops.end())
    loop = it.value();

  if (enable) {
    loop.enabled = true;
    mWaveFormFullView->add_loop(id, loop.start_frame, loop.end_frame);
    mWaveFormZoomedView->add_loop(id, loop.start_frame, loop.end_frame);
  } else {
    mWaveFormFullView->remove_loop(id);
    mWaveFormZoomedView->remove_loop(id);
    loop.enabled = false;
  }
  mLoops[id] = loop;
}

