#include "db.hpp"
#include "defines.hpp"
#include <stdexcept>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QHash>
#include <QVariant>
#include <QSqlError>
#include <QSqlDriver>
#include <QFileInfo>
#include <QStringList>
#include <QSqlTableModel>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace {
   QSqlDatabase cDB;
   QMutex mMutex(QMutex::Recursive);
   bool has_transactions = false;

   const QString cFileQuery(
         "SELECT audio_file_location, annotation_file_location\n"
         "FROM audio_works\n"
         "WHERE audio_works.id = :id");

   const QString cWorkInfoQuery(
         "SELECT audio_works.name title,\n"
         "\tartists.name artist \n"
         "FROM `audio_works`\n"
         "\tINNER JOIN `artists` ON `artists`.`id` = `audio_works`.`artist_id`\n"
         "WHERE audio_works.id = :id");

   const QString cWorkInsert(
         "INSERT INTO audio_works\n"
         "(name, year, audio_file_type_id, audio_file_location, audio_file_milliseconds, audio_file_channels, artist_id)\n"
         "VALUES\n"
         "(:name, :year, :audio_file_type_id, :audio_file_location, :audio_file_milliseconds, :audio_file_channels, :artist_id)\n"
         );

   const QString cWorkIDFromLocation("SELECT id FROM audio_works where audio_works.audio_file_location = :audio_file_location");

   const QString cArtistFind("SELECT id FROM artists where name = :name");
   const QString cArtistInsert("INSERT INTO artists (name) values (:name)");

   const QString cAlbumFind("SELECT id FROM albums where name = :name");
   const QString cAlbumInsert("INSERT INTO albums (name) values (:name)");
   const QString cAlbumInsertTrack(
         "INSERT INTO album_audio_works\n"
         "(album_id, audio_work_id, track)\n"
         "values (:album_id, :audio_work_id, :track)\n");

   const QString cFileTypeFind("SELECT id FROM audio_file_types where name = :name");
   const QString cFileTypeInsert("INSERT INTO audio_file_types (name) values (:name)");

   const QString cDescriptorTypesAll("SELECT id, name FROM descriptor_types");
   const QString cDescriptorTypeFind("SELECT id FROM descriptor_types WHERE name = :name");
   const QString cDescriptorTypeCreate("INSERT INTO descriptor_types (name) VALUES ( :name )");
   const QString cDescriptorFindByWorkId("SELECT id FROM descriptors WHERE descriptor_type_id = :descriptor_type_id AND audio_work_id = :audio_work_id");
   const QString cDescriptorUpdateValueById("UPDATE descriptors SET value = :value WHERE id = :id");
   const QString cDescriptorInsert("INSERT INTO descriptors (descriptor_type_id, audio_work_id, value) VALUES(:descriptor_type_id, :audio_work_id, :value)");

   int cWorkArtistIdColumn = 0;
   int cWorkAlbumIdColumn = 0;
   int cWorkAudioFileTypeIdColumn = 0;

	bool setup_query(
			QSqlQuery& query,
			const QString& query_string, 
			const QHash<QString, QVariant>& bindValues) {
		if(!query.prepare(query_string))
			return false;

		QHashIterator<QString, QVariant> i(bindValues);
		while (i.hasNext())
			query.bindValue(i.key(), i.value());

		return true;
	}

   void build_work_table() throw(std::runtime_error) {
      QMutexLocker lock(&mMutex);
      //build up query
      QSqlQuery query(cDB);

      QStringList selects;
      QStringList joins;

      selects << "audio_works.*";
      joins << "audio_works";

      selects << "albums.id AS album_id";
      selects << "album_audio_works.track AS track";
      joins << "INNER JOIN album_audio_works ON album_audio_works.audio_work_id = audio_works.id INNER JOIN albums ON albums.id = album_audio_works.album_id";

      //build up descriptor joins
      if(!query.prepare(cDescriptorTypesAll) || !query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to query: " + query.lastError().text().toStdString()));
      while(query.next()) {
         QString name = query.value(1).toString();
         QString id = query.value(0).toString();
         joins << "LEFT JOIN descriptors as `" + name + "` ON audio_works.id = `" + name + "`.audio_work_id AND `" + name + "`.descriptor_type_id = " + id;
         selects << "`" + name + "`.value as `" + name + "`";
      }

      QString query_string = "CREATE TEMPORARY TABLE works AS SELECT " + selects.join(", ") + " FROM " + joins.join(" ");
      
      //actually create the table
      if(!query.prepare(query_string) || !query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to query: " + query.lastError().text().toStdString()));

      QSqlTableModel tab;
      tab.setTable("works");
      cWorkArtistIdColumn = tab.fieldIndex("artist_id");
      cWorkAlbumIdColumn = tab.fieldIndex("album_id");
      cWorkAudioFileTypeIdColumn = tab.fieldIndex("audio_file_type_id");
   };
}

using namespace dj::model;

void db::setup(
      QString type, 
      QString name, 
      QString username,
      QString password,
      int /* port */,
      QString /* host */) throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);

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

   build_work_table();
}

QSqlDatabase db::get() { return cDB; }
void db::close() { cDB.close(); }

bool db::find_locations_by_id(
		int work_id,
		QString& audio_file_loc,
		QString& annotation_file_loc) {
   QMutexLocker lock(&mMutex);

   //build up query
	QSqlQuery query(get());
	if(!query.prepare(cFileQuery))
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
   QMutexLocker lock(&mMutex);

   //build up query
	QSqlQuery query(get());
	if(!query.prepare(cWorkInfoQuery))
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

int db::work::temp_table_id_column(QString id_name) {
   if (id_name == "artist")
      return cWorkArtistIdColumn;
   else if (id_name == "album")
      return cWorkAlbumIdColumn;
   else if (id_name == "audio_file_type")
      return cWorkAudioFileTypeIdColumn;
   return 0;
}

int db::work::create(
		const QHash<QString, QVariant>& attributes,
		const QString& audio_file_location
		) throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);

	int work_id = 0;
   QSqlDriver * db_driver = get().driver();

   try {
      int album_id = 0;
      QHash<QString, QVariant>::const_iterator i;

      if (has_transactions)
         db_driver->beginTransaction();

      QSqlQuery query(get());
      if(!query.prepare(cWorkInsert))
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));

      query.bindValue(":audio_file_location", audio_file_location);

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
      throw;
   }

   return work_id;
}

int db::work::find_by_audio_file_location(
      const QString& audio_file_location) throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);

	QSqlQuery query(get());
	if(!query.prepare(cWorkIDFromLocation))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":audio_file_location", audio_file_location);

   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   if (!query.first())
      return 0;
   return query.value(0).toInt();
}

void db::work::update_attribute(
      int work_id,
      const QString& name,
      const QVariant& value) {
   QMutexLocker lock(&mMutex);

	QSqlQuery query(get());

   QString update_string("UPDATE audio_works SET " + name + " = :value WHERE audio_works.id = :id");

   //try to find an artist by the same name
	if(!query.prepare(update_string))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));

   query.bindValue(":value", value);
   query.bindValue(":id", work_id);

   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
}

void db::work::descriptor_create_or_update(
		int work_id,
		const QString& descriptor_type_name,
		double value) {
   QMutexLocker lock(&mMutex);
	QSqlQuery query(get());

   //find the descriptor type named
	if(!query.prepare(cDescriptorTypeFind))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", descriptor_type_name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));

   //if it was found, get the id, otherwise create a descriptor type with this name
   int descriptor_type_id = 0;
   if (query.first())
      descriptor_type_id = query.value(0).toInt();
   else {
      if(!query.prepare(cDescriptorTypeCreate))
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
      query.bindValue(":name", descriptor_type_name);
      if (!query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
      descriptor_type_id = query.lastInsertId().toInt();
   }

   //see if there is an existing descriptor to update
   if(!query.prepare(cDescriptorFindByWorkId))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));

   query.bindValue(":descriptor_type_id", descriptor_type_id);
   query.bindValue(":audio_work_id", work_id);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));

   if (query.first()) {
      int descriptor_id = query.value(0).toInt();
      if(!query.prepare(cDescriptorUpdateValueById))
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
      query.bindValue(":id", descriptor_id);
      query.bindValue(":value", value);

      if (!query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   } else {
      if(!query.prepare(cDescriptorInsert))
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
      query.bindValue(":descriptor_type_id", descriptor_type_id);
      query.bindValue(":audio_work_id", work_id);
      query.bindValue(":value", value);
      if (!query.exec())
         throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   }

}

void db::work::tag(
		int work_id,
		const QString& tag_class,
		const QString& tag_value) {
   QMutexLocker lock(&mMutex);
   QSqlQuery query(get());
}

int db::artist::find(const QString& name, bool create) throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);
	QSqlQuery query(get());

   //try to find an artist by the same name
	if(!query.prepare(cArtistFind))
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
   if(!query.prepare(cArtistInsert))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}

int db::album::find(const QString& name, bool create)  throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cAlbumFind))
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
   if(!query.prepare(cAlbumInsert))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}

void db::album::add_work(int album_id, int work_id, int track_num)  throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cAlbumInsertTrack))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":album_id", album_id);
   query.bindValue(":audio_work_id", work_id);
   query.bindValue(":track", track_num);

   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
}

int db::file_type::find(const QString& name, bool create) throw(std::runtime_error) {
   QMutexLocker lock(&mMutex);
	QSqlQuery query(get());

   //try to find an album by the same name
	if(!query.prepare(cFileTypeFind))
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
   if(!query.prepare(cFileTypeInsert))
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to prepare query: " + query.lastError().text().toStdString()));
   query.bindValue(":name", name);
   if (!query.exec())
      throw(std::runtime_error(DJ_FILEANDLINE + " failed to execute query: " + query.lastError().text().toStdString()));
   return query.lastInsertId().toInt();
}
