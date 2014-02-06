#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
  class MainWindow;
}

class DB;
class AudioModel;
class AudioLoader;
class WorkFilterModelCollection;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public:
    explicit MainWindow(DB* db, AudioModel * audio, QWidget *parent = 0);
    void loader(AudioLoader * loader);
    ~MainWindow();
  public slots:
    void readSettings();
    void writeSettings();
    void masterSetValueInt(QString name, int v);
  protected slots:
    void selectWork(int id);
  signals:
    void workSelected(int workid);

  private:
    void addFilterTab(QString filterExpression = QString(), QString title = "filtered");
    Ui::MainWindow *ui;
    DB * mDB;
    WorkFilterModelCollection * mFilterCollection;
};

#endif // MAINWINDOW_H
