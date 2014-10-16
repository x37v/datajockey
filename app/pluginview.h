#ifndef AUDIO_PLUGIN_VIEW_H
#define AUDIO_PLUGIN_VIEW_H

#include <QWidget>
#include "plugin.h"

class AudioPluginView : public QWidget {
  Q_OBJECT
  public:
    static AudioPluginView * createView(AudioPlugin * plugin, QWidget * parent = nullptr);
    AudioPluginView(QWidget * parent = nullptr);
};

#endif

