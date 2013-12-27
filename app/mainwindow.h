#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
  class MainWindow;
}

class DB;
class AudioModel;
class AudioLoader;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public:
    explicit MainWindow(DB* db, AudioModel * audio, QWidget *parent = 0);
    void loader(AudioLoader * loader);
    ~MainWindow();

  private:
    Ui::MainWindow *ui;
    DB * mDB;
};

#endif // MAINWINDOW_H
