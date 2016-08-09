#include "pluginmixerview.h"
#include "ui_pluginmixerview.h"

PluginMixerView::PluginMixerView(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PluginMixerView)
{
  ui->setupUi(this);
}

PluginMixerView::~PluginMixerView()
{
  delete ui;
}
