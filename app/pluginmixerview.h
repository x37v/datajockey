#ifndef PLUGINMIXERVIEW_H
#define PLUGINMIXERVIEW_H

#include <QWidget>

namespace Ui {
  class PluginMixerView;
}

class PluginMixerView : public QWidget
{
    Q_OBJECT

  public:
    explicit PluginMixerView(QWidget *parent = 0);
    ~PluginMixerView();

  private:
    Ui::PluginMixerView *ui;
};

#endif // PLUGINMIXERVIEW_H
