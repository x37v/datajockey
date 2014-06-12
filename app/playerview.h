#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include <QWidget>
#include <QString>
#include "defines.hpp"

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

    void jumpUpdate(dj::loop_and_jump_type_t type, int entry_index, int frame_start, int frame_end);
    void jumpsClear();
    void jumpClear(int entry_index);
  signals:
    void valueChangedDouble(QString name, double v);
    void valueChangedInt(QString name, int v);
    void valueChangedBool(QString name, bool v);
    void triggered(QString name);

  private:
    Ui::PlayerView *ui;
};

#endif // PLAYERVIEW_H
