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

int db::work::create(
		const QMap<QString, QVariant>& attributes,
		const QString& audio_file_location,
		const QString& annotation_file_location
		) {
	int artist_id = 0;
	int album_id = 0;
	int audio_file_id = 0;
	int annotation_file_id = 0;

	QMap<QString, QVariant>::const_iterator i;

	i = attributes.find("artist");
	if (i != attributes.end())
		artist_id = artist::find(i.value().toString(), true);

	i = attributes.find("album");
	if (i != attributes.end())
		album_id = album::find(i.value().toString(), true);

	audio_file_id = audio_file::create(audio_file_location);
	annotation_file_id = annotation_file::create(annotation_file_location);

	return 0;
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

int db::audio_file::create(const QString& location) {
	return 0;
}

QString db::audio_file::location_by_work_id(int work_id) {
	return QString("");
}

int db::annotation_file::create(const QString& location) {
	return 0;
}

QString db::annotation_file::location_by_work_id(int work_id) {
	return QString("");
}

int db::artist::find(const QString& name, bool create) {
	return 0;
}

int db::album::find(const QString& name, bool create) {
	return 0;
}
