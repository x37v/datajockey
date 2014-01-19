#ifndef MIXERPANELVIEW_H
#define MIXERPANELVIEW_H

#include <QWidget>
#include <QList>
#include "audiobuffer.hpp"
#include "annotation.hpp"

namespace Ui {
  class MixerPanelView;
}

class PlayerView;

class MixerPanelView : public QWidget
{
  Q_OBJECT

  public:
    explicit MixerPanelView(QWidget *parent = 0);
    ~MixerPanelView();
  public slots:
    void playerSetValueInt(int player, QString name, int value);
    void playerSetValueBool(int player, QString name, bool value);
    void playerSetValueDouble(int player, QString name, double value);
    void playerSetValueString(int player, QString name, QString value);
    void playerSetBuffers(int player, djaudio::AudioBufferPtr audio_buffer, djaudio::BeatBufferPtr beat_buffer);

    void masterSetValueInt(QString name, int value);
    void masterSetValueBool(QString name, bool value);
    void masterSetValueDouble(QString name, double value);

  signals:
    void playerValueChangedDouble(int player, QString name, double v);
    void playerValueChangedInt(int player, QString name, int v);
    void playerValueChangedBool(int player, QString name, bool v);
    void playerTriggered(int player, QString name);

    void masterValueChangedDouble(QString name, double v);
    void masterValueChangedInt(QString name, int v);
    void masterValueChangedBool(QString name, bool v);
    void masterTriggered(QString name);

  private:
    Ui::MixerPanelView *ui;
    QList<PlayerView *> mPlayerViews;

    bool inRange(int player);
};

#endif // MIXERPANELVIEW_H
