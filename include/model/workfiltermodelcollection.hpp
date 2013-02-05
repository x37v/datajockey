#ifndef WORK_FILTER_MODEL_COLLECTION_HPP
#define WORK_FILTER_MODEL_COLLECTION_HPP

#include <QString>
#include <QObject>
#include <QSqlDatabase>
#include <QDateTime>

class WorkFilterModel;
class QTimer;

class WorkFilterModelCollection : public QObject {
  Q_OBJECT

  public:
    WorkFilterModelCollection(QObject * parent = NULL, QSqlDatabase db = QSqlDatabase());
    WorkFilterModel * new_filter_model(QObject * parent = NULL);

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

    void update_history(int work_id, int session_id, QDateTime played_at); //just a relay

  signals:
    void work_selected(int work);
    void current_bpm_changed(double bpm);
    void updated_history(int work_id, int session_id, QDateTime played_at);

  protected slots:
    void bpm_send_timeout();

  private:
    double mCurrentBPM;
    double mLastBPM;
    QTimer * mBPMTimeout;
};

#endif
