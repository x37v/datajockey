#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <QObject>
#include <QHash>

class AudioPluginManager {
  Q_OBJECT
  public slots:
    void playerSetValueInt(int player, QString name, int value);
  signals:
    void pluginValueChangedDouble(int plugin_index, QString parameter_name, int value);
    void pluginValueChangedBool(int plugin_index, QString parameter_name, bool value);
  private:
    QHash<int, int> mPlayerEQPluginIndices;
};

#endif
