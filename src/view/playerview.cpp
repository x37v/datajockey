#include "playerview.hpp"
#include "defines.hpp"
#include "waveformviewgl.hpp"
#include "audiobufferreference.hpp"
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
   }
   //sync is checked to start out with
   mButtons["sync"]->setChecked(true);

   button_layout->setSpacing(0);
   button_layout->setColumnStretch(0, 100);
   button_layout->setColumnStretch(4, 100);
   mControlLayout->addLayout(button_layout);

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

   mAudioLevelView = new AudioLevel(this);

   mSliderLevelLayout = new QBoxLayout(QBoxLayout::LeftToRight);
   mSliderLevelLayout->addStretch(10);
   mSliderLevelLayout->addWidget(mVolumeSlider, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addWidget(mAudioLevelView, 1, Qt::AlignHCenter);
   mSliderLevelLayout->addStretch(10);
   mSliderLevelLayout->setContentsMargins(0,0,0,0);

   mControlLayout->addLayout(mSliderLevelLayout);
   mControlLayout->setContentsMargins(0,0,0,0);

   mWaveFormView = new WaveFormViewGL(this, true);
   mWaveFormView->setVisible(waveform_orientation != WAVEFORM_NONE);
   mWaveFormView->setMinimumWidth(220);

   mTopLayout->addLayout(mControlLayout, 0);
   mTopLayout->addWidget(mWaveFormView, 10);
   mTopLayout->setContentsMargins(0,0,0,0);
   setLayout(mTopLayout);

   QObject::connect(mWaveFormView,
         SIGNAL(seek_relative(int)),
         SLOT(relay_seek_relative(int)));
   QObject::connect(mWaveFormView,
         SIGNAL(mouse_down(bool)),
         SLOT(relay_mouse_button(bool)));
   QObject::connect(mButtons["sync"],
         SIGNAL(toggled(bool)),
         mSpeedView,
         SLOT(setDisabled(bool)));
}

QPushButton * Player::button(QString name) const { return mButtons[name]; }
QList<QPushButton *> Player::buttons() const { return mButtons.values(); }
QDial * Player::eq_dial(QString name) const { return mEqDials[name]; }
QList<QDial *> Player::eq_dials() const { return mEqDials.values(); }
QSlider * Player::volume_slider() const { return mVolumeSlider; }
QSpinBox * Player::speed_view() const { return mSpeedView; }
QProgressBar * Player::progress_bar() const { return mProgressBar; }
QRect Player::slider_level_geometry() const { return mSliderLevelLayout->geometry(); }

void Player::set_audio_level(int percent) { mAudioLevelView->set_level(percent); }

void Player::set_audio_file(const QString& file_name) {
   audio::AudioBufferReference ref(file_name);

   mWaveFormView->set_audio_file(file_name);
   if (ref.valid())
      mFrames = ref->length();
   else
      mFrames = 0;
   mProgressBar->setValue(0);
}

void Player::set_audio_frame(int frame) {
   mWaveFormView->set_frame(frame);

   if (mFrames) {
      mProgressBar->setValue(frame / (mFrames / 100));
   } else
      mProgressBar->setValue(0);
}

void Player::set_beat_buffer(audio::BeatBuffer buffer) {
   mWaveFormView->set_beat_buffer(buffer);
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

void Player::relay_seek_relative(int frames) { emit(seek_frame_relative(frames)); }
void Player::relay_mouse_button(bool down) { emit(seeking(down)); }

