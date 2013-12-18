#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include <QWidget>
#include <QString>

namespace Ui {
  class PlayerView;
}

class PlayerView : public QWidget
{
  Q_OBJECT

  public:
    explicit PlayerView(QWidget *parent = 0);
    ~PlayerView();
  public slots:
    void setWorkInfo(QString info);

    void setValueDouble(QString name, double v);
    void setValueInt(QString name, int v);
    void setValueBool(QString name, bool v);
  signals:
    void valueChangedDouble(QString name, double v);
    void valueChangedInt(QString name, int v);
    void valueChangedBool(QString name, bool v);
    void triggered(QString name);

  private:
    Ui::PlayerView *ui;
};

#endif // PLAYERVIEW_H
