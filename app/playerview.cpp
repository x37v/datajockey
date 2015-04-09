#include "playerview.h"
#include "ui_playerview.h"
#include "audiolevelview.h"
#include "loopandjumpcontrolview.h"
#include <QDoubleSpinBox>
#include <map>

PlayerView::PlayerView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PlayerView)
{
  ui->setupUi(this);

  connect(ui->speedBox,
      static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
      [this] (double v) { emit(valueChangedDouble("speed", v)); });

  ui->speedBox->setEnabled(false);

  //connect up ui elements
  std::map<QAbstractSlider *, QString> sliders = {
    {ui->volume, "volume"},
    {ui->eqHigh, "eq_high"},
    {ui->eqMid, "eq_mid"},
    {ui->eqLow, "eq_low"},
  };
  for (auto kv: sliders) {
    QString name = kv.second;
    connect(kv.first, &QSlider::valueChanged,
        [this, name] (int v) { emit(valueChangedInt(name, v)); });
  }

  std::map<QToolButton *, QString> checkableBtns = {
    {ui->cue, "cue"},
    {ui->playPause, "play"},
    {ui->sync, "sync"}
  };
  for (auto kv: checkableBtns) {
    QString name = kv.second;
    connect(kv.first, &QToolButton::toggled,
        [this, name] (bool v) { emit(valueChangedBool(name, v)); });
  }

  std::map<QToolButton *, QString> nonCheckableBtns = {
    {ui->load, "load"},
    {ui->seekFwd, "seek_fwd"},
    {ui->seekBack, "seek_back"},
    {ui->bumpFwd, "bump_fwd"},
    {ui->bumpBack, "bump_back"},
    {ui->jump, "jump"}
  };

  for (auto kv: nonCheckableBtns) {
    QString name = kv.second;
    connect(kv.first, &QToolButton::pressed,
        [this, name] () { 
          emit(triggered(name));
          emit(valueChangedBool(name + "_down", true));
        });
    connect(kv.first, &QToolButton::released,
        [this, name] () { emit(valueChangedBool(name + "_down", false)); });
  }
  connect(ui->jump_up, &QToolButton::released, [this] () { emit(triggered("jump")); });

  connect(ui->loopAndJumpControl, &LoopAndJumpControlView::valueChangedInt, this, &PlayerView::valueChangedInt);
  connect(ui->loopAndJumpControl, &LoopAndJumpControlView::triggered, this, &PlayerView::triggered);
}

void PlayerView::setWorkInfo(QString info) {
  ui->workInfo->setText(info);
}

void PlayerView::setValueDouble(QString name, double v) {
  if (name == "speed")
    ui->speedBox->setValue(v);
  else if (name == "audio_level")
    ui->level->setLevel(static_cast<int>(100.0 * v));
}

void PlayerView::setValueInt(QString name, int v) {
  if (name == "volume")
    ui->volume->setValue(v);
  else if (name == "eq_high")
    ui->eqHigh->setValue(v);
  else if (name == "eq_mid")
    ui->eqMid->setValue(v);
  else if (name == "eq_low")
    ui->eqLow->setValue(v);
  else if (name == "load_percent")
    ui->progressBar->setValue(v);
}

void PlayerView::setValueBool(QString name, bool v) {
  if (name == "cue")
    ui->cue->setChecked(v);
  else if (name == "play")
    ui->playPause->setChecked(v);
  else if (name == "sync") {
    ui->sync->setChecked(v);
    ui->speedBox->setEnabled(!v);
  }
  ui->loopAndJumpControl->setValueBool(name, v);
}

void PlayerView::jumpUpdate(dj::loop_and_jump_type_t type, int entry_index, int /*frame_start*/, int /*frame_end*/) {
  ui->loopAndJumpControl->updateEntry(type, entry_index);
}

void PlayerView::jumpsClear() {
  ui->loopAndJumpControl->clearAll();
}

void PlayerView::jumpClear(int entry_index) {
  ui->loopAndJumpControl->clearEntry(entry_index);
}

PlayerView::~PlayerView()
{
  delete ui;
}
