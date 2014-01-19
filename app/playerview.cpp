#include "playerview.h"
#include "ui_playerview.h"
#include "audiolevelview.h"
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
    {ui->bumpBack, "bump_back"}
  };
  for (auto kv: nonCheckableBtns) {
    QString name = kv.second;
    connect(kv.first, &QToolButton::clicked,
        [this, name] () { emit(triggered(name)); });
  }
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
  else if (name == "sync")
    ui->sync->setChecked(v);
}

PlayerView::~PlayerView()
{
  delete ui;
}
