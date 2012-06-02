#include "db.hpp"
#include "defines.hpp"
#include <stdexcept>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
#include <QVariant>
#include <QSqlError>
#include <QSqlDriver>
#include <QFileInfo>

namespace {
   QSqlDatabase cDB;
   bool has_transactions = false;

   const QString cFileQueryString(
         "SELECT audio_file_location, annotation_file_location\n"
         "FROM audio_works\n"
         "WHERE audio_works.id = :id");

   const QString cWorkInfoQueryString(
         "SELECT audio_works.name title,\n"
         "\tartists.name artist \n"
         "FROM `audio_works`\n"
         "\tINNER JOIN `artists` ON `artists`.`id` = `audio_works`.`artist_id`\n"
         "WHERE audio_works.id = :id");

   const QString cWorkInsertString(
         "INSERT INTO audio_works\n"
         "(name, year, audio_file_type_id, audio_file_location, audio_file_milliseconds, audio_file_channels, annotation_file_location, artist_id)\n"
         "VALUES\n"
         "(:name, :year, :audio_file_type_id, :audio_file_location, :audio_file_milliseconds, :audio_file_channels, :annotation_file_location, :artist_id)\n"
         );

   const QString cArtistFindString("SELECT id FROM artists where name = :name");
   const QString cArtistInsertString("INSERT INTO artists (name) values (:name)");

   const QString cAlbumFindString("SELECT id FROM albums where name = :name");
   const QString cAlbumInsertString("INSERT INTO albums (name) values (:name)");
   const QString cAlbumInsertTrackString(
         "INSERT INTO album_audio_works\n"
         "(album_id, audio_work_id, track)\n"
         "values (:album_id, :audio_work_id, :track)\n");

   const QString cFileTypeFindString("SELECT id FROM audio_file_types where name = :name");
   const QString cFileTypeInsertString("INSERT INTO audio_file_types (name) values (:name)");

	bool setup_query(
			QSqlQuery& query,
			const QString& query_string, 
			const QMap<QString, QVariant>& bindValues) {
		if(!query.prepare(query_string))
			return false;

		QMapIterator<QString, QVariant> i(bindValues);
		while (i.hasNext())
			query.bindValue(i.key(), i.value());

		return true;
	}

}

using namespace DataJockey::Model;

void db::setup(
      QString type, 
      QString name, 
      QString username,
      QString password,
      int /* port */,
      QString /* host */) throw(std::runtime_error) {

	cDB = QSqlDatabase::addDatabase(type);
	cDB.setDatabaseName(name);
	if(!username.isEmpty())
		cDB.setUserName(username);
	if(!password.isEmpty())
		cDB.setPassword(password);
	if(!cDB.open())
		throw std::runtime_error("cannot open database");

   if(cDB.driver()->hasFeature(QSqlDriver::Transactions))
      has_transactions = true;
}

QSqlDatabase db::get() { return cDB; }
void db::close() { cDB.close(); }

bool db::find_locations_by_id(
		int work_id,
		QString& audio_file_loc,
		QString& annotation_file_loc) {

   //build up query
	QSqlQuery query(get());
	if(!query.prepare(cFileQueryString))
		return false;
	query.bindValue(":id", work_id);

   //execute
   if(!(query.exec() && query.first()))
		return false;

   QSqlRecord rec = query.record();
   int audioFileCol = rec.indexOf("audio_file_location");
   int annotationFileCol = rec.indexOf("annotation_file_location");

	audio_file_loc = query.value(audioFileCol).toString();
   annotation_file_loc = query.value(annotationFileCol).toString();
	return true;
}

bool db::find_artist_and_title_by_id(
		int work_id,
		QString& artist_name,
		QString& work_title) {

   //build up query
	QSqlQuery query(get());
	if(!query.prepare(cWorkInfoQueryString))
		return false;
	query.bindValue(":id", work_id);

   //execute
   if(!(query.exec() && query.first()))
		return false;

   QSqlRecord rec = query.record();
	int titleCol = rec.indexOf("title");
	int artistCol = rec.indexOf("artist");

	artist_name = query.value(artistCol).toString();
	work_title = query.value(titleCol).toString();

	return true;
}

int db::work::create(
		const QMap<QString, QVariant>& attributes,
		const QString& audio_file_location,
		const QString& annotation_file_location
		) throw(std::runtime_error) {
	int work_id = 0;
   QSqlDriver * db_driver = get().driver();

   try {
      int album_id = 0;
      QMap<QString, QVariant>::const_iterator i;

      if (has_transactions)
         db_driver->beginTransaction();

      QSqlQuery query(get());
      if(!query.prepare(cWorkInsertString))
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));

      query.bindValue(":audio_file_location", audio_file_location);
      query.bindValue(":annotation_file_location", annotation_file_location);

      i = attributes.find("name");
      if (i != attributes.end())
         query.bindValue(":name", i.value());
      else
         throw(std::runtime_error("you must provide a work name"));

      i = attributes.find("year");
      if (i != attributes.end())
         query.bindValue(":year", i.value());
      else
         query.bindValue(":year", "NULL");

      i = attributes.find("channels");
      if (i != attributes.end())
         query.bindValue(":audio_file_channels", i.value());
      else
         query.bindValue(":audio_file_channels", 2);

      i = attributes.find("milliseconds");
      if (i != attributes.end())
         query.bindValue(":audio_file_milliseconds", i.value());
      else
         query.bindValue(":audio_file_milliseconds", "NULL");

      i = attributes.find("file_type");
      QString file_type_name;
      if (i != attributes.end())
         file_type_name = i.value().toString();
      else
         file_type_name = QFileInfo(audio_file_location).suffix().toLower();
      query.bindValue(":audio_file_type_id", file_type::find(file_type_name, true));

      i = attributes.find("artist_id");
      if (i != attributes.end())
         query.bindValue(":artist_id", i.value());
      else {
         i = attributes.find("artist");
         if (i != attributes.end()) {
            query.bindValue(":artist_id", artist::find(i.value().toString(), true));
         } else {
            query.bindValue(":artist_id", "NULL");
         }
      }

      if (!query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
      work_id = query.lastInsertId().toInt();

      i = attributes.find("album");
      if (i != attributes.end()) {
         album_id = album::find(i.value().toString(), true);
         i = attributes.find("track");
         int track_num = (i != attributes.end()) ? i.value().toInt() : 0;
         db::album::add_work(album_id, work_id, track_num);
      }
      if (has_transactions) {
         if (!db_driver->commitTransaction())
            db_driver->rollbackTransaction();
      }
   } catch (std::exception& e) {
      if (has_transactions)
         db_driver->rollbackTransaction();
      throw e;
   }

   return work_id;
}

void db::work::descriptor_create_or_update(
		int work_id,
		const QString& descriptor_type,
		float value) {
}

void db::work::tag(
		int work_id,
		const QString& tag_class,
		const QString& tag_value) {
}

int db::artist::find(const QString& name, bool create) throw(std::runtime_error) {
	QSqlQuery query(get());

   //try to find an artist by the same name
	if(!query.prepare(cArtistFindString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));

   //if it is found, return the index
   if (query.first())
      return query.value(0).toInt();

   //otherwise, if we aren't creating a new record, return 0
   if (!create)
      return 0;

   //otherwise, create a new artist
   if(!query.prepare(cArtistInsertString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}

int db::album::find(const QString& name, bool create)  throw(std::runtime_error) {
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cAlbumFindString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));

   //if it is found, return the index
   if (query.first())
      return query.value(0).toInt();

   //otherwise, if we aren't creating a new record, return 0
   if (!create)
      return 0;

   //otherwise, create a new album
   if(!query.prepare(cAlbumInsertString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}

void db::album::add_work(int album_id, int work_id, int track_num)  throw(std::runtime_error) {
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cAlbumInsertTrackString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":album_id", album_id);
   query.bindValue(":audio_work_id", work_id);
   query.bindValue(":track", track_num);

   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
}

int db::file_type::find(const QString& name, bool create) throw(std::runtime_error) {
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cFileTypeFindString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));

   //if it is found, return the index
   if (query.first())
      return query.value(0).toInt();

   //otherwise, if we aren't creating a new record, return 0
   if (!create)
      return 0;

   //otherwise, create a new album
   if(!query.prepare(cFileTypeInsertString))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}
