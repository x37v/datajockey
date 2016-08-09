#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QHash>
#include "plugin.h"

class AudioPluginManager : public QObject {
  Q_OBJECT
  public:
    static AudioPluginManager * instance();
    AudioPluginPtr create(QString uniqueId);
    //only call once you've ditched all instances from elsewhere
    void destroy(AudioPluginPtr plugin);
  public slots:
    //relay info to the eq
    void playerSetValueInt(int player, QString name, int value);
  signals:
    void pluginValueChangedDouble(int plugin_index, QString parameter_name, int value);
    void pluginValueChangedBool(int plugin_index, QString parameter_name, bool value);
  private:
    QHash<int, AudioPluginPtr> mPlugins; //keep track of the instance so the audio model can use raw pointers
    AudioPluginManager();
};

#endif
