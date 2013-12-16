#include "playerview.h"
#include "ui_playerview.h"
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

PlayerView::~PlayerView()
{
  delete ui;
}
