#include "workrelationmodel.hpp"
#include "db.hpp"

using namespace dj;

WorkRelationModel::WorkRelationModel(const QString& table_name, QObject * parent, QSqlDatabase db) : 
  QSqlRelationalTableModel(parent, db)
{
  setTable(table_name);
  setRelation(model::db::work::temp_table_id_column("audio_file_type"), QSqlRelation("audio_file_types", "id", "name"));
  setRelation(model::db::work::temp_table_id_column("artist"), QSqlRelation("artists", "id", "name"));
  setRelation(model::db::work::temp_table_id_column("album"), QSqlRelation("albums", "id", "name"));
}

