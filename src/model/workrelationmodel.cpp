#include "workrelationmodel.hpp"
#include "db.hpp"

using namespace dj;

WorkRelationModel::WorkRelationModel(const QString& table_name, QObject * parent, QSqlDatabase db) : 
  QSqlRelationalTableModel(parent, db)
{
  setTable(table_name);
  setRelation(model::db::work::temp_table_id_column("audio_file_type"), QSqlRelation("audio_file_types", "id", "name"));
  setHeaderData(model::db::work::temp_table_id_column("audio_file_type"), Qt::Horizontal, "file_type", Qt::DisplayRole);

  setRelation(model::db::work::temp_table_id_column("artist"), QSqlRelation("artists", "id", "name"));
  setHeaderData(model::db::work::temp_table_id_column("artist"), Qt::Horizontal, "artist", Qt::DisplayRole);

  setRelation(model::db::work::temp_table_id_column("album"), QSqlRelation("albums", "id", "name"));
  setHeaderData(model::db::work::temp_table_id_column("album"), Qt::Horizontal, "album", Qt::DisplayRole);

}

