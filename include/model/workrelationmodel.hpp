#ifndef WORK_RELATION_MODEL_HPP
#define WORK_RELATION_MODEL_HPP

#include <QSqlRelationalTableModel>
#include <QString>

//convenience class, sets up the relationships for artist, album and audio file types
class WorkRelationModel : public QSqlRelationalTableModel {
  Q_OBJECT
  public:
    WorkRelationModel(const QString& table_name, QObject * parent = NULL, QSqlDatabase db = QSqlDatabase());
    void setup_relations();
};

#endif
