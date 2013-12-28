#ifndef MIXERPANELVIEW_H
#define MIXERPANELVIEW_H

#include <QWidget>
#include <QList>

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
    void playerSetWorkInfo(int player, QString info);

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
};

#endif // MIXERPANELVIEW_H
