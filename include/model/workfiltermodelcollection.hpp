#ifndef WORK_FILTER_MODEL_COLLECTION_HPP
#define WORK_FILTER_MODEL_COLLECTION_HPP

#include <QString>
#include <QObject>
#include <QSqlDatabase>
#include <QList>

class WorkFilterModel;

class WorkFilterModelCollection : public QObject {
  Q_OBJECT

  public:
    WorkFilterModelCollection(QObject * parent = NULL, QSqlDatabase db = QSqlDatabase());
    WorkFilterModel * new_filter_model();

  public slots:
    void player_trigger(int player_index, QString name);
    void player_set(int player_index, QString name, bool value);
    void player_set(int player_index, QString name, int value);
    void player_set(int player_index, QString name, double value);
    void player_set(int player_index, QString name, QString value);

    void master_trigger(QString name);
    void master_set(QString name, bool value);
    void master_set(QString name, int value);
    void master_set(QString name, double value);

    void select_work(int work_id); //just a relay

  signals:
    void work_selected(int work);
    void current_bpm_changed(double bpm);

  private:
    double mCurrentBPM;
    QList<WorkFilterModel *> mFilterModels;
};

#endif
