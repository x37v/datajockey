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
#include <QDir>
#include <QStringList>
#include <QSqlTableModel>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QRegExp>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

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
      "(name, year, audio_file_type_id, audio_file_location, audio_file_seconds, audio_file_channels, artist_id, created_at, updated_at)\n"
      "VALUES\n"
      "(:name, :year, :audio_file_type_id, :audio_file_location, :audio_file_seconds, :audio_file_channels, :artist_id, :created_at, :updated_at)\n"
      );

  const QString cWorkHistoryInsert(
      "INSERT INTO audio_work_histories\n"
      "(audio_work_id, session_id, played_at)\n"
      "VALUES\n"
      "(:audio_work_id, :session_id, :played_at)"
      );

  const QString cSessionQuery("SELECT distinct session_id FROM audio_work_histories ORDER BY session_id DESC");
  const QString cWorksSessionUpdate("UPDATE audio_works SET last_session_id = :last_session_id, last_played_at = :last_played_at WHERE id = :work_id");

  const QString cWorkIDFromLocation("SELECT id FROM audio_works where audio_works.audio_file_location = :audio_file_location");

  const QString cArtistFind("SELECT id FROM artists where name = :name");
  const QString cArtistInsert("INSERT INTO artists (name) values (:name)");

  const QString cAlbumFind("SELECT id FROM albums where name = :name");
  const QString cAlbumInsert("INSERT INTO albums (name) values (:name)");
  const QString cWorkUpdateAlbum(
      "UPDATE audio_works SET album_id = :album_id, album_track = :track \n"
      "WHERE audio_works.id = :audio_work_id");

  const QString cFileTypeFind("SELECT id FROM audio_file_types where name = :name");
  const QString cFileTypeInsert("INSERT INTO audio_file_types (name) values (:name)");

  const QString cDescriptorTypesAll("SELECT id, name FROM descriptor_types");
  const QString cDescriptorTypeFind("SELECT id FROM descriptor_types WHERE name = :name");
  const QString cDescriptorTypeCreate("INSERT INTO descriptor_types (name) VALUES ( :name )");
  const QString cDescriptorFindByWorkId("SELECT id FROM descriptors WHERE descriptor_type_id = :descriptor_type_id AND audio_work_id = :audio_work_id");
  const QString cDescriptorUpdateValueById("UPDATE descriptors SET value = :value WHERE id = :id");
  const QString cDescriptorInsert("INSERT INTO descriptors (descriptor_type_id, audio_work_id, value) VALUES(:descriptor_type_id, :audio_work_id, :value)");

  const QString cTagClassFind("SELECT id FROM tag_classes WHERE name = :name");
  const QString cTagClassCreate("INSERT INTO tag_classes (name) VALUES (:name)");

  const QString cTagFind("SELECT id FROM tags WHERE name = :name AND tag_class_id = :tag_class_id");
  const QString cTagFindNoClass("SELECT id FROM tags WHERE name = :name");
  const QString cTagCreate("INSERT INTO tags (name, tag_class_id) VALUES (:name, :tag_class_id)");

  const QString cWorkTagFind("SELECT id FROM audio_work_tags WHERE tag_id = :tag_id AND audio_work_id = :audio_work_id");
  const QString cWorkTagCreate("INSERT INTO audio_work_tags (tag_id, audio_work_id) VALUES (:tag_id, :audio_work_id)");

  QStringList work_table_selects;
  QStringList work_table_joins;

  int cWorkIdColumn = 0;
  int cWorkSongLengthColumn = 6;
  int cWorkSessionIdColumn = 8;

  const QString cWorkTableSelectColumns(
      " w.id, "
      "artists.name as artist, "
      "w.name, "
      "albums.name as album, "
      "w.album_track as track, "
      "w.descriptor_tempo_median as tempo_median, "
      "w.audio_file_seconds as seconds, "
      "w.last_played_at, "
      "w.last_session_id, "
      "w.year, "
      "audio_file_types.name as file_type, "
      "w.created_at");

  int cFilteredWorkTableCount = 0;

  int cCurrentSession = 0;

  //this class simply wraps the prepare and exec methods of QSqlQuery so it throws exceptions on failures
  class MySqlQuery : public QSqlQuery {
    public:
      MySqlQuery(QSqlDatabase db) : QSqlQuery(db) {
      }
      bool prepare(const QString & query){
        if (!QSqlQuery::prepare(query))
          throw(std::runtime_error("failed to query: " + lastError().text().toStdString()));
        return true;
      }
      bool exec() {
        if (!QSqlQuery::exec())
          throw(std::runtime_error("failed to exec: " + lastError().text().toStdString()));
        return true;
      }
  };

  void update_temp_work_table(const QString& table_name, const QString& where_clause = QString()) throw(std::runtime_error) {
    QMutexLocker lock(&mMutex);
    MySqlQuery query(cDB);

    //delete everything from the table
    QString query_string = "DELETE FROM " + table_name;
    query.prepare(query_string);
    query.exec();

    //prepare the insert
    query_string = "INSERT INTO " + table_name + " SELECT DISTINCT audio_works.*" +
      "FROM audio_Works LEFT OUTER JOIN audio_work_tags ON audio_works.id = audio_work_tags.audio_work_id";
    if (!where_clause.isEmpty())
      query_string += (" WHERE " + where_clause);
    query_string += " ORDER BY album_id, album_track";

    //actually create the table
    query.prepare(query_string);
    query.exec();
  }

}

using namespace dj::model;

void db::setup(
    QString type, 
    QString name_or_loc, 
    QString username,
    QString password,
    int /* port */,
    QString /* host */) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);

  //create an empty sqlite db if it doesn't already exist at name_or_loc
  if (type == "QSQLITE") {
    QFileInfo file_info(name_or_loc);
    if (!file_info.exists()) {
      QDir dir(file_info.dir());
      if (!dir.exists()) {
        if (!dir.mkpath(dir.path()))
          throw(std::runtime_error("cannot create path " + dir.path().toStdString()));
      }
      QFile db_file(":/resources/datajockey.sqlite3");
      if (!db_file.copy(name_or_loc) || !QFile::setPermissions(name_or_loc, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner))
        throw(std::runtime_error("cannot create writable sqlite db :" + name_or_loc.toStdString()));
    }
  }

  cDB = QSqlDatabase::addDatabase(type);
  cDB.setDatabaseName(name_or_loc);
  if(!username.isEmpty())
    cDB.setUserName(username);
  if(!password.isEmpty())
    cDB.setPassword(password);
  if(!cDB.open())
    throw std::runtime_error("cannot open database");

  if(cDB.driver()->hasFeature(QSqlDriver::Transactions))
    has_transactions = true;

  //find the current session
  try {
    MySqlQuery query(get());
    query.prepare(cSessionQuery);
    if(query.exec() && query.first())
      cCurrentSession = query.value(0).toInt() + 1;
  } catch (std::runtime_error e) {
  }
}

QSqlDatabase db::get() { return cDB; }
void db::close() {
  cDB.close();
  cDB = QSqlDatabase();
}

bool db::find_locations_by_id(
    int work_id,
    QString& audio_file_loc,
    QString& annotation_file_loc) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);

  //build up query
  MySqlQuery query(get());
  query.prepare(cFileQuery);
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
    QString& work_title) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);

  //build up query
  MySqlQuery query(get());
  query.prepare(cWorkInfoQuery);
  query.bindValue(":id", work_id);

  //execute
  query.exec();
  if(!query.first())
    return false;

  QSqlRecord rec = query.record();
  int titleCol = rec.indexOf("title");
  int artistCol = rec.indexOf("artist");

  artist_name = query.value(artistCol).toString();
  work_title = query.value(titleCol).toString();

  return true;
}

QString db::work::table_query(const QString where_clause, const QString session_clause) throw(std::runtime_error) {
  QString from(" FROM audio_works as w");

  QString joins = QString(" LEFT JOIN albums ON w.album_id = albums.id") +
      " LEFT JOIN artists ON w.artist_id = artists.id" +
      " LEFT JOIN audio_file_types ON w.audio_file_type_id = audio_file_types.id";

  //COALESCE fills a 0 in in case of NULL
  QString query_string;
  if (where_clause.isEmpty()) {
    query_string = QString("SELECT") + cWorkTableSelectColumns + from + joins;
  } else {
    query_string = QString("SELECT DISTINCT ") + cWorkTableSelectColumns + from
       + joins +
      " LEFT JOIN audio_work_tags ON w.id = audio_work_tags.audio_work_id";
    query_string += " WHERE " + where_clause;
  }
  
  query_string += " ORDER BY artists.name, albums.name, w.album_track, w.name";
  return query_string;
}

int db::work::table_column(QString id_name) {
  if (id_name == "id")
    return cWorkIdColumn;
  else if (id_name == "audio_file_seconds")
    return cWorkSongLengthColumn;
  else if (id_name == "session")
    return cWorkSessionIdColumn;
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

    MySqlQuery query(get());
    query.prepare(cWorkInsert);

    query.bindValue(":audio_file_location", audio_file_location);

    QDateTime now = QDateTime::currentDateTime();
    query.bindValue(":created_at", now);
    query.bindValue(":updated_at", now);

    i = attributes.find("name");
    if (i != attributes.end())
      query.bindValue(":name", i.value());
    else
      throw(std::runtime_error("you must provide a work name"));

    i = attributes.find("year");
    if (i != attributes.end())
      query.bindValue(":year", i.value());
    else
      query.bindValue(":year", QVariant::Int);

    i = attributes.find("channels");
    if (i != attributes.end())
      query.bindValue(":audio_file_channels", i.value());
    else
      query.bindValue(":audio_file_channels", 2);

    i = attributes.find("seconds");
    if (i != attributes.end())
      query.bindValue(":audio_file_seconds", i.value());
    else
      query.bindValue(":audio_file_seconds", QVariant::Int);

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
        query.bindValue(":artist_id", QVariant::Int);
      }
    }

    query.exec();
    work_id = query.lastInsertId().toInt();

    i = attributes.find("album");
    if (i != attributes.end()) {
      album_id = album::find(i.value().toString(), true);
      i = attributes.find("track");
      int track_num = (i != attributes.end()) ? i.value().toInt() : 0;
      db::work::set_album(work_id, album_id, track_num);
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

  MySqlQuery query(get());
  query.prepare(cWorkIDFromLocation);
  query.bindValue(":audio_file_location", audio_file_location);
  query.exec();

  if (!query.first())
    return 0;
  return query.value(0).toInt();
}

void db::work::update_attribute(
    int work_id,
    const QString& name,
    const QVariant& value) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);

  MySqlQuery query(get());

  QString update_string("UPDATE audio_works SET " + name + " = :value WHERE audio_works.id = :id");

  //try to find an artist by the same name
  query.prepare(update_string);

  query.bindValue(":value", value);
  query.bindValue(":id", work_id);

  query.exec();
}

void db::work::descriptor_create_or_update(
    int work_id,
    const QString& descriptor_type_name,
    double value) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //find the descriptor type named
  query.prepare(cDescriptorTypeFind);
  query.bindValue(":name", descriptor_type_name);
  query.exec();

  //if it was found, get the id, otherwise create a descriptor type with this name
  int descriptor_type_id = 0;
  if (query.first())
    descriptor_type_id = query.value(0).toInt();
  else {
    query.prepare(cDescriptorTypeCreate);
    query.bindValue(":name", descriptor_type_name);
    query.exec();
    descriptor_type_id = query.lastInsertId().toInt();
  }

  //see if there is an existing descriptor to update
  query.prepare(cDescriptorFindByWorkId);
  query.bindValue(":descriptor_type_id", descriptor_type_id);
  query.bindValue(":audio_work_id", work_id);
  query.exec();

  if (query.first()) {
    int descriptor_id = query.value(0).toInt();
    query.prepare(cDescriptorUpdateValueById);
    query.bindValue(":id", descriptor_id);
    query.bindValue(":value", value);
    query.exec();
  } else {
    query.prepare(cDescriptorInsert);
    query.bindValue(":descriptor_type_id", descriptor_type_id);
    query.bindValue(":audio_work_id", work_id);
    query.bindValue(":value", value);
    query.exec();
  }
}

void db::work::tag(
    int work_id,
    const QString& tag_class,
    const QString& tag_value) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //find the tag class named
  query.prepare(cTagClassFind);
  query.bindValue(":name", tag_class);
  query.exec();

  //if it was found, get the id, otherwise create one
  int tag_class_id = 0;
  if (query.first())
    tag_class_id = query.value(0).toInt();
  else {
    query.prepare(cTagClassCreate);
    query.bindValue(":name", tag_class);
    query.exec();
    tag_class_id = query.lastInsertId().toInt();
  }

  //find the tag, if it exists
  query.prepare(cTagFind);
  query.bindValue(":name", tag_value);
  query.bindValue(":tag_class_id", tag_class_id);
  query.exec();

  int tag_id = 0;
  if (query.first()) {
    tag_id = query.value(0).toInt();

    //see if the work already has this tag
    query.prepare(cWorkTagFind);
    query.bindValue(":tag_id", tag_id);
    query.bindValue(":audio_work_id", work_id);
    query.exec();
    if (query.first())
      return;  //the work already has this tag
  } else {
    query.prepare(cTagCreate);
    query.bindValue(":name", tag_value);
    query.bindValue(":tag_class_id", tag_class_id);
    query.exec();
    tag_id = query.lastInsertId().toInt();
  }

  query.prepare(cWorkTagCreate);
  query.bindValue(":tag_id", tag_id);
  query.bindValue(":audio_work_id", work_id);
  query.exec();
}

void db::work::set_played(int work_id, int session_id, QDateTime time) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  try {
    query.prepare(cWorkHistoryInsert);
    query.bindValue(":work_id", work_id);
    query.bindValue(":session_id", session_id);
    query.bindValue(":played_at", time);
    query.exec();
  } catch (std::runtime_error e) {
    //XXX what's up?
  }

  try {
    query.prepare(cWorksSessionUpdate);
    query.bindValue(":work_id", work_id);
    query.bindValue(":last_session_id", session_id);
    query.bindValue(":last_played_at", time);
    query.exec();
  } catch (std::runtime_error e) {
    cerr << "failed to update audio works table" << endl;
  }
}

int db::work::current_session() { return cCurrentSession; }

void db::work::set_album(int work_id, int album_id, int track_num)  throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  query.prepare(cWorkUpdateAlbum);
  query.bindValue(":album_id", album_id);
  query.bindValue(":audio_work_id", work_id);
  query.bindValue(":album_track", track_num);
  query.exec();
}


int db::tag::find_class(const QString& name) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //find the tag class named
  query.prepare(cTagClassFind);
  query.bindValue(":name", name);
  query.exec();

  if (query.first())
    return query.value(0).toInt();
  throw std::runtime_error("cannot find tag class with name " + name.toStdString());
}

int db::tag::find(const QString& name, int tag_class_id) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //if the tag_class_id is >= 0 we include the class in the search
  if (tag_class_id >= 0) {
    query.prepare(cTagFind);
    query.bindValue(":tag_class_id", tag_class_id);
  } else {
    query.prepare(cTagFindNoClass);
  }

  query.bindValue(":name", name);
  query.exec();
  if (query.first())
    return query.value(0).toInt();
  throw std::runtime_error("cannot find tag with name " + name.toStdString());
}

int db::artist::find(const QString& name, bool create) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //try to find an artist by the same name
  query.prepare(cArtistFind);
  query.bindValue(":name", name);
  query.exec();

  //if it is found, return the index
  if (query.first())
    return query.value(0).toInt();

  //otherwise, if we aren't creating a new record, return 0
  if (!create)
    return 0;

  //otherwise, create a new artist
  query.prepare(cArtistInsert);
  query.bindValue(":name", name);
  query.exec();
  return query.lastInsertId().toInt();
}

int db::album::find(const QString& name, bool create)  throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //try to find an album by the same name
  query.prepare(cAlbumFind);
  query.bindValue(":name", name);
  query.exec();

  //if it is found, return the index
  if (query.first())
    return query.value(0).toInt();

  //otherwise, if we aren't creating a new record, return 0
  if (!create)
    return 0;

  //otherwise, create a new album
  query.prepare(cAlbumInsert);
  query.bindValue(":name", name);
  query.exec();
  return query.lastInsertId().toInt();
}

int db::file_type::find(const QString& name, bool create) throw(std::runtime_error) {
  QMutexLocker lock(&mMutex);
  MySqlQuery query(get());

  //try to find an album by the same name
  query.prepare(cFileTypeFind);
  query.bindValue(":name", name);
  query.exec();

  //if it is found, return the index
  if (query.first())
    return query.value(0).toInt();

  //otherwise, if we aren't creating a new record, return 0
  if (!create)
    return 0;

  //otherwise, create a new album
  query.prepare(cFileTypeInsert);
  query.bindValue(":name", name);
  query.exec();
  return query.lastInsertId().toInt();
}

