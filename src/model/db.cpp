#include "db.hpp"
#include <stdexcept>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMap>
#include <QVariant>

namespace {
   QSqlDatabase cDB;

   const QString cFileQueryString(
         "select audio_files.location audio_file, annotation_files.location annotation_file\n"
         "from audio_works\n"
         "\tjoin audio_files on audio_files.id = audio_works.audio_file_id\n"
         "\tjoin annotation_files on annotation_files.audio_work_id = audio_works.id\n"
         "where audio_works.id = :id");

   const QString cWorkInfoQueryString(
         "select audio_works.name title,\n"
         "\tartists.name artist \n"
         "from audio_works"
         "\tinner join artist_audio_works on artist_audio_works.audio_work_id = audio_works.id\n"
         "\tinner join artists on artists.id = artist_audio_works.artist_id\n"
         "where audio_works.id = :id");

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
      int port,
      QString host) {

	cDB = QSqlDatabase::addDatabase(type);
	cDB.setDatabaseName(name);
	if(!username.isEmpty())
		cDB.setUserName(username);
	if(!password.isEmpty())
		cDB.setPassword(password);
	if(!cDB.open())
		throw std::runtime_error("cannot open database");
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
   int audioFileCol = rec.indexOf("audio_file");
   int annotationFileCol = rec.indexOf("annotation_file");

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
