#include "mixerpanelview.h"
#include "ui_mixerpanelview.h"
#include "playerview.h"

MixerPanelView::MixerPanelView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::MixerPanelView)
{
  ui->setupUi(this);
  mPlayerViews.push_back(ui->leftPlayer);
  mPlayerViews.push_back(ui->rightPlayer);

  //connect up the players
  for (int i = 0; i < mPlayerViews.size(); i++) {
    PlayerView * player = mPlayerViews[i];
    connect(player, 
        &PlayerView::valueChangedDouble,
        [this, i] (QString name, double v) { emit(playerValueChangedDouble(i, name, v)); });
    connect(player, 
        &PlayerView::valueChangedInt,
        [this, i] (QString name, int v) { emit(playerValueChangedInt(i, name, v)); });
    connect(player, 
        &PlayerView::valueChangedBool,
        [this, i] (QString name, bool v) { emit(playerValueChangedBool(i, name, v)); });
    connect(player, 
        &PlayerView::triggered,
        [this, i] (QString name) { emit(playerTriggered(i, name)); });
  }

  connect(ui->syncToPlayer0, &QPushButton::clicked, [this] { emit(masterTriggered("sync_to_player0")); });
  connect(ui->syncToPlayer1, &QPushButton::clicked, [this] { emit(masterTriggered("sync_to_player1")); });

  connect(ui->volume, &QSlider::valueChanged,
      [this] (int value) { emit(masterValueChangedInt("volume", value)); });
  connect(ui->tempoBox, 
      static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
      [this] (double value) { emit(masterValueChangedDouble("bpm", value)); });

  connect(ui->waveforms, &MixerPanelWaveformsView::playerValueChangedBool,
      this, &MixerPanelView::playerValueChangedBool);
  connect(ui->waveforms, &MixerPanelWaveformsView::playerValueChangedInt,
      this, &MixerPanelView::playerValueChangedInt);
}

MixerPanelView::~MixerPanelView()
{
  delete ui;
}

void MixerPanelView::playerSetValueString(int player, QString name, QString value) {
  if (!inRange(player))
    return;
  if (name == "loading_work") {
    mPlayerViews[player]->setWorkInfo("loading: " + value);
    ui->waveforms->playerSetBuffers(player, djaudio::AudioBufferPtr(), djaudio::BeatBufferPtr());
  } else if (name == "work_info")
    mPlayerViews[player]->setWorkInfo(value);
}

void MixerPanelView::playerSetValueInt(int player, QString name, int value) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->setValueInt(name, value);
  ui->waveforms->playerSetValueInt(player, name, value);
}

void MixerPanelView::playerSetValueBool(int player, QString name, bool value) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->setValueBool(name, value);
}

void MixerPanelView::playerSetValueDouble(int player, QString name, double value) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->setValueDouble(name, value);
}

void MixerPanelView::playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer) {
  if (!inRange(player))
    return;
  ui->waveforms->playerSetBuffers(player, audio_buffer, beat_buffer);
}

void MixerPanelView::jumpUpdate(int player, dj::loop_and_jump_type_t type, int entry_index, int frame_start, int frame_end) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->jumpUpdate(type, entry_index, frame_start, frame_end);
  ui->waveforms->updateMarker(player, type, entry_index, frame_start, frame_end);
}

void MixerPanelView::jumpsClear(int player) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->jumpsClear();
  ui->waveforms->clearAllMarkers(player);
}

void MixerPanelView::jumpClear(int player, int entry_index) {
  if (!inRange(player))
    return;
  mPlayerViews[player]->jumpClear(entry_index);
  ui->waveforms->clearMarker(player, entry_index);
}

void MixerPanelView::masterSetValueInt(QString name, int value) {
}

void MixerPanelView::masterSetValueBool(QString name, bool value) {
}

void MixerPanelView::masterSetValueDouble(QString name, double value) {
  if (name == "audio_level")
    ui->level->setLevel(static_cast<int>(value * 100.0));
  else if (name == "bpm")
    ui->tempoBox->setValue(value);
}

bool MixerPanelView::inRange(int player) {
  return player >= 0 && player < mPlayerViews.size();
}

