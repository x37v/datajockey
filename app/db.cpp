#include "db.h"
//#include "defines.hpp"
#include "config.hpp"
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
#include <QDateTime>
#include <QRegExp>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;

namespace {
  bool has_transactions = false;

  const QString cFileQuery(
      "SELECT audio_file_location, annotation_file_location\n"
      "FROM audio_works\n"
      "WHERE audio_works.id = :id");

  const QString cWorkInfoQuery(
      "SELECT audio_works.name title,\n"
      "\tartists.name artist, \n"
      "\talbums.name album \n"
      "FROM `audio_works`\n"
      "\tINNER JOIN `artists` ON `artists`.`id` = `audio_works`.`artist_id`\n"
   "\tLEFT JOIN albums ON `albums`.`id` = `audio_works`.`album_id`\n"
      "\tWHERE audio_works.id = :id");

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
      "(:work_id, :session_id, :played_at)"
      );

  const QString cSessionQuery("SELECT distinct session_id FROM audio_work_histories ORDER BY session_id DESC");
  const QString cWorksSessionUpdate("UPDATE audio_works SET last_session_id = :last_session_id, last_played_at = :last_played_at WHERE id = :work_id");

  const QString cWorkIDFromLocation("SELECT id FROM audio_works where audio_works.audio_file_location = :audio_file_location");

  const QString cArtistFind("SELECT id FROM artists where name = :name");
  const QString cArtistInsert("INSERT INTO artists (name) values (:name)");

  const QString cAlbumFind("SELECT id FROM albums where name = :name");
  const QString cAlbumInsert("INSERT INTO albums (name) values (:name)");
  const QString cWorkUpdateAlbum(
      "UPDATE audio_works SET album_id = :album_id, album_track = :album_track \n"
      "WHERE audio_works.id = :audio_work_id");

  const QString cFileTypeFind("SELECT id FROM audio_file_types where name = :name");
  const QString cFileTypeInsert("INSERT INTO audio_file_types (name) values (:name)");

  const QString cTagFind("SELECT id FROM tags WHERE name = :name AND parent_id = :parent_id");
  const QString cTagFindNoParent("SELECT id FROM tags WHERE name = :name");
  const QString cTagCreate("INSERT INTO tags (name, parent_id) VALUES (:name, :parent_id)");

  const QString cWorkTagFind("SELECT id FROM audio_work_tags WHERE tag_id = :tag_id AND audio_work_id = :audio_work_id");
  const QString cWorkTagCreate("INSERT INTO audio_work_tags (tag_id, audio_work_id) VALUES (:tag_id, :audio_work_id)");
  const QString cWorkTagRemove("DELETE FROM audio_work_tags WHERE tag_id = :tag_id AND audio_work_id = :audio_work_id");

  QStringList work_table_selects;
  QStringList work_table_joins;

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

  int cCurrentSession = 0;
  QStringList cDescriptorTypes;

  //this class simply wraps the prepare and exec methods of QSqlQuery so it throws exceptions on failures
  class MySqlQuery : public QSqlQuery {
    public:
      MySqlQuery(QSqlDatabase db) : QSqlQuery(db) {
      }
      bool prepare(const QString & query){
        if (!QSqlQuery::prepare(query))
          throw(std::runtime_error("failed to query: " + QSqlQuery::lastQuery().toStdString() + " error:" + lastError().text().toStdString()));
        return true;
      }
      bool exec() {
        if (!QSqlQuery::exec())
          throw(std::runtime_error("failed to exec: " + QSqlQuery::lastQuery().toStdString() + " error:" + lastError().text().toStdString()));
        return true;
      }
  };

  /*
  void update_temp_work_table(const QString& table_name, const QString& where_clause = QString()) throw(std::runtime_error) {
    MySqlQuery query(mDB);

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
  */

  QString fmt_string(const QString& input) {
    QString ret = input.toLower();
    ret = ret.replace(QRegExp("\\s\\s*"), "_");
    ret = ret.replace('\\', '_');
    ret = ret.replace('/', '_');
    ret = ret.replace('~', '_');
    ret = ret.replace(QRegExp("\\.+"), "_");
    return QDir::cleanPath(ret);
  }

  QString default_file_location(int work_id, QHash<QString, QVariant> tag_data) {
    QDir dir(dj::Configuration::instance()->annotation_dir());
    QString file_name;
    file_name.setNum(work_id);

    QHash<QString, QVariant>::const_iterator i;
    i = tag_data.find("artist");
    if (i != tag_data.end())
      file_name.append("-" + fmt_string(i->toString()));

    i = tag_data.find("album");
    if (i != tag_data.end()) {
      QHash<QString, QVariant> album = i->toHash();
      QHash<QString, QVariant>::const_iterator j = album.find("name");
      if (j != album.end())
        file_name.append("-" + fmt_string(j->toString()));
    }

    i = tag_data.find("name");
    if (i != tag_data.end())
      file_name.append("-" + fmt_string(i->toString()));

    file_name.append(".yaml");
    return dir.filePath(file_name);
  }
}

Tag::Tag(int id, QString name) :
  mID(id),
  mName(name)
{
}

Tag::~Tag() {
  if (mParent)
    mParent->removeChild(this);
  while (mChilden.size()) {
    Tag * child = mChilden.first();
    mChilden.removeAt(0);
    delete child;
  }
}

int Tag::childIndex(Tag * tag) const {
  return mChilden.indexOf(tag);
}

void Tag::appendChild(Tag * tag) {
  tag->parent(this);
  mChilden.append(tag);
}

void Tag::removeChild(Tag * tag) {
  int index = childIndex(tag);
  if (index >= 0) {
    mChilden[index]->parent(nullptr);
    mChilden.removeAt(index);
  }
}

Tag * Tag::removeChildAt(int index) {
  if (index >= mChilden.size())
    return nullptr;
  Tag * child = mChilden[index];
  child->parent(nullptr);
  mChilden.removeAt(index);
  return child;
}

Tag * Tag::child(int index) {
  if (index < mChilden.size())
    return mChilden[index];
  return nullptr;
}

bool Tag::hasChild(Tag * child, bool recurse) {
  for (int i = 0; i < mChilden.size(); i++) {
    Tag * c = mChilden[i];
    if (c->id() == child->id())
      return true;
    if (recurse && c->hasChild(child, true))
      return true;
  }
  return false;
}

DB::DB(
    QString type, 
    QString name_or_loc, 
    QString username,
    QString password,
    int /* port */,
    QString /* host */,
    QObject * parent
    ) throw(std::runtime_error) : QObject(parent) { 
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
        throw(std::runtime_error("cannot create writable sqlite db: " + name_or_loc.toStdString()));
    }
  }

  mDB = QSqlDatabase::addDatabase(type);
  mDB.setDatabaseName(name_or_loc);
  if(!username.isEmpty())
    mDB.setUserName(username);
  if(!password.isEmpty())
    mDB.setPassword(password);
  if(!mDB.open())
    throw std::runtime_error("cannot open database");

  if(mDB.driver()->hasFeature(QSqlDriver::Transactions))
    has_transactions = true;

  //find the current session
  try {
    MySqlQuery query(get());
    query.prepare(cSessionQuery);
    if(query.exec() && query.first())
      cCurrentSession = query.value(0).toInt() + 1;
  } catch (std::runtime_error e) {
    cerr << "query failed, couldn't set current session: " << e.what() << endl;
  }

  //XXX update descriptor list from columns
  cDescriptorTypes << "tempo_median";
  cDescriptorTypes << "tempo_average";
}

DB::~DB() {
  close();
}

QSqlDatabase DB::get() { return mDB; }

void DB::close() {
  mDB.close();
  mDB = QSqlDatabase();
}

bool DB::find_locations_by_id(
    int work_id,
    QString& audio_file_loc,
    QString& annotation_file_loc) throw(std::runtime_error) {
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

void DB::format_string_by_id(int work_id, QString& info) throw(std::runtime_error) {
  //build up query
  MySqlQuery query(get());
  query.prepare(cWorkInfoQuery);
  query.bindValue(":id", work_id);

  //execute
  if(!(query.exec() && query.first()))
    return;

  QSqlRecord rec = query.record();
  info.replace("$artist", query.value(rec.indexOf("artist")).toString());
  info.replace("$title", query.value(rec.indexOf("title")).toString());
  info.replace("$album", query.value(rec.indexOf("album")).toString());
}

bool DB::find_artist_and_title_by_id(
    int work_id,
    QString& artist_name,
    QString& work_title) throw(std::runtime_error) {
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

QString DB::work_table_query(const QString where_clause) throw(std::runtime_error) {
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

int DB::work_table_column(QString name) {
  if (name == "id")
    return WORK_ID;
  else if (name == "artist")
    return WORK_ARTIST_NAME;
  else if (name == "name")
    return WORK_NAME;
  else if (name == "album")
    return WORK_ALBUM_NAME;
  else if (name == "track")
    return WORK_ALBUM_TRACK;
  else if (name == "tempo")
    return WORK_TEMPO;
  else if (name == "audio_file_seconds")
    return WORK_SONG_LENGTH;
  else if (name == "last_played_at")
    return WORK_LAST_PLAYED;
  else if (name == "session")
    return WORK_SESSION_ID;
  return -1;
}

int DB::work_create(
    const QHash<QString, QVariant>& attributes,
    const QString& audio_file_location
    ) throw(std::runtime_error) {
  int work_id = 0;
  QSqlDriver * db_driver = get().driver();

  try {
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
    query.bindValue(":audio_file_type_id", file_type_find(file_type_name, true));

    i = attributes.find("artist_id");
    if (i != attributes.end())
      query.bindValue(":artist_id", i.value());
    else {
      i = attributes.find("artist");
      if (i != attributes.end()) {
        query.bindValue(":artist_id", artist_find(i.value().toString(), true));
      } else {
        query.bindValue(":artist_id", QVariant::Int);
      }
    }

    query.exec();
    work_id = query.lastInsertId().toInt();

    i = attributes.find("album");
    if (i != attributes.end()) {
      int track_num = 0;
      int album_id = 0;
      //hash or flat style
      if (i.value().canConvert(QMetaType::QVariantHash)) {
        QHash<QString, QVariant> ahash = i.value().toHash();
        i = ahash.find("name");
        if (i != ahash.end())
          album_id = album_find(i.value().toString(), true);
        i = ahash.find("track");
        track_num = (i != attributes.end()) ? i.value().toInt() : 0;
      } else { 
        if (i != attributes.end())
          album_id = album_find(i.value().toString(), true);
        i = attributes.find("track");
        track_num = (i != attributes.end()) ? i.value().toInt() : 0;
      }
      DB::work_set_album(work_id, album_id, track_num);
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

int DB::work_find_by_audio_file_location(
    const QString& audio_file_location) throw(std::runtime_error) {
  MySqlQuery query(get());
  query.prepare(cWorkIDFromLocation);
  query.bindValue(":audio_file_location", audio_file_location);
  query.exec();

  if (!query.first())
    return 0;
  return query.value(0).toInt();
}

void DB::work_update_attribute(
    int work_id,
    const QString& name,
    const QVariant& value) throw(std::runtime_error) {
  MySqlQuery query(get());

  QString update_string = QString("UPDATE audio_works SET %1=:value WHERE audio_works.id = :id").arg(name);

  //try to find an artist by the same name
  query.prepare(update_string);

  query.bindValue(":value", value);
  query.bindValue(":id", work_id);

  query.exec();
}

QVariant DB::work_attribute(
    int work_id,
    const QString& name) throw(std::runtime_error) {
  QString sel = QString("SELECT %1 FROM audio_works WHERE audio_works.id = :id").arg(name);
  MySqlQuery query(get());
  query.prepare(sel);
  query.bindValue(":id", work_id);
  query.exec();

  if (!query.first())
    return QVariant();
  return query.value(0);
}

void DB::work_descriptor_create_or_update(
    int work_id,
    QString name,
    double value) throw(std::runtime_error) {

  name.replace(QRegExp("\\s"), "_"); //replace any spaces with underscore
  if (!cDescriptorTypes.contains(name))
    throw std::runtime_error("invalid descriptor type: " + name.toStdString());

  QString column_name = QString("descriptor_%1").arg(name);
  work_update_attribute(work_id, column_name, value);
}

/*
void DB::work_tag(
    int work_id,
    const QString& tag_class,
    const QString& tag_value) throw(std::runtime_error) {
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
*/

void DB::work_tag(int work_id, int tag_id) throw(std::runtime_error) {
  MySqlQuery query(get());

  query.prepare(cWorkTagFind);
  query.bindValue(":tag_id", tag_id);
  query.bindValue(":audio_work_id", work_id);
  query.exec();
  if (query.first())
    return;  //the work already has this tag

  query.prepare(cWorkTagCreate);
  query.bindValue(":tag_id", tag_id);
  query.bindValue(":audio_work_id", work_id);
  query.exec();
}

void DB::work_tag_remove(int work_id, int tag_id) throw(std::runtime_error) {
  MySqlQuery query(get());
  query.prepare(cWorkTagRemove);
  query.bindValue(":tag_id", tag_id);
  query.bindValue(":audio_work_id", work_id);
  query.exec();
}

void DB::work_set_played(int work_id, QDateTime time) {
  MySqlQuery query(get());

  try {
    query.prepare(cWorkHistoryInsert);
    query.bindValue(":work_id", work_id);
    query.bindValue(":session_id", cCurrentSession);
    query.bindValue(":played_at", time);
    query.exec();
  } catch (std::runtime_error e) {
    cerr << "failed to update sessions table: " << e.what() << endl;
  }

  try {
    query.prepare(cWorksSessionUpdate);
    query.bindValue(":work_id", work_id);
    query.bindValue(":last_session_id", cCurrentSession);
    query.bindValue(":last_played_at", time);
    query.exec();
  } catch (std::runtime_error e) {
    cerr << "failed to update audio works table: " << e.what() << endl;
  }
}

void DB::import(QString audioFilePath, QString annotationFilePath, QHash<QString, QVariant> tagData) {
  try {
    //import file
    int id = work_find_by_audio_file_location(audioFilePath);
    QString movedAnnotation;
    bool update_existing = false;

    if (id == 0) {
      id = work_create(tagData, audioFilePath);
      movedAnnotation = default_file_location(id, tagData);
    } else {
      update_existing = true;
      QString dummyAudioPath;
      if (!find_locations_by_id(id, dummyAudioPath, movedAnnotation)) {
        update_existing = false;
        movedAnnotation = default_file_location(id, tagData);
      }
    }

    //move the annotation
    QFileInfo movedInfo(movedAnnotation);
    QDir dir = movedInfo.dir();
    if (!dir.mkpath(dir.path()))
      throw std::runtime_error("couldn't create path to annotation file: " + movedAnnotation.toStdString());
    if (update_existing)
      QFile::remove(movedAnnotation);
    if (!QFile::rename(annotationFilePath, movedAnnotation))
      throw std::runtime_error("couldn't move to annotation file to: " + movedAnnotation.toStdString());

    //add the annotation
    work_update_attribute(id, "annotation_file_location", movedAnnotation);

    //add the median tempo
    auto it = tagData.find("tempo_median");
    if (it != tagData.end())
      work_descriptor_create_or_update(id, "tempo_median", it.value().toDouble());
  } catch (std::runtime_error& e) {
    emit(importError(audioFilePath, QString::fromStdString(e.what())));
    return;
  }
  emit(importSuccess(audioFilePath));
}

int DB::current_session() { return cCurrentSession; }

void DB::work_set_album(int work_id, int album_id, int track_num)  throw(std::runtime_error) {
  MySqlQuery query(get());

  query.prepare(cWorkUpdateAlbum);
  query.bindValue(":album_id", album_id);
  query.bindValue(":audio_work_id", work_id);
  query.bindValue(":album_track", track_num);
  query.exec();
}

QString DB::work_jump_data(int work_id) {
  MySqlQuery query(get());

  query.prepare("SELECT data from audio_work_jumps where audio_work_id=:audio_work_id");
  query.bindValue(":audio_work_id", work_id);
  query.exec();
  if (query.first())
    return query.value(0).toString();
  return QString();
}

void DB::work_jump_data(int work_id, QString data) {
  MySqlQuery query(get());
  if (data.size() == 0) {
    query.prepare("DELETE FROM audio_work_jumps WHERE audio_work_id=:audio_work_id");
  } else {
    query.prepare("INSERT OR REPLACE INTO audio_work_jumps (data, audio_work_id) VALUES(:data, :audio_work_id)");
    query.bindValue(":data", data);
  }
  query.bindValue(":audio_work_id", work_id);
  query.exec();
}

int DB::tag_find(const QString& name, int parent_id) throw(std::runtime_error) {
  MySqlQuery query(get());

  //if the parent_id is > 0 we include it in the search
  if (parent_id > 0) {
    query.prepare(cTagFind);
    query.bindValue(":parent_id", parent_id);
  } else {
    query.prepare(cTagFindNoParent);
  }

  query.bindValue(":name", name);
  query.exec();
  if (query.first())
    return query.value(0).toInt();
  throw std::runtime_error("cannot find tag with name " + name.toStdString());
}

QList<Tag*> DB::tags(int work_id) {
  QString queryString = "SELECT tags.name, tags.id, tags.parent_id, tparent.name"
    " FROM tags "
    " LEFT OUTER JOIN tags AS tparent ON tparent.id = tags.parent_id";
  if (work_id) {
    queryString += " JOIN audio_work_tags on tags.id = audio_work_tags.tag_id";
    queryString += " WHERE audio_work_tags.audio_work_id = " + QString::number(work_id);
  }
  queryString += " ORDER BY tags.parent_id, tags.name";

  MySqlQuery query(get());
  query.prepare(queryString);
  query.exec();

  QHash<int, Tag *> top_tags;
  while(query.next()) {
    QString tag_name = query.value(0).toString();
    int tag_id = query.value(1).toInt();

    Tag * tag = new Tag(tag_id, tag_name);

    int parent_id = query.value(2).toInt();
    if (parent_id) {
      Tag * tag_parent;
      if (!top_tags.contains(parent_id)) {
        QString parent_name = query.value(3).toString();
        tag_parent = new Tag(parent_id, parent_name);
        top_tags[parent_id] = tag_parent;
      } else {
        tag_parent = top_tags[parent_id];
      }
      tag_parent->appendChild(tag);
    } else {
      top_tags[tag->id()] = tag;
    }
  }

  return top_tags.values();
}

bool DB::tag_exists(QString name, Tag * parent) {
  MySqlQuery query(get());
  QString queryString = "SELECT id FROM tags WHERE name = :name AND parent_id = :parent_id";
  query.prepare(queryString);
  query.bindValue(":parent_id", parent ? parent->id() : 0);
  query.bindValue(":name", name);
  query.exec();
  return query.first();
}

Tag * DB::tag_create(QString name, Tag * parent) {
  MySqlQuery query(get());
  QString queryString = "INSERT INTO tags (name, parent_id) VALUES (:name, :parent_id)";
  query.prepare(queryString);
  query.bindValue(":name", name);
  query.bindValue(":parent_id", parent ? parent->id() : 0);
  query.exec();

  int id = query.lastInsertId().toInt();
  Tag * tag = new Tag(id, name);
  if (parent)
    parent->appendChild(tag);
  return tag;
}

void DB::tag_destroy(Tag * tag) {
  while (tag->children().size()) {
    Tag * child = tag->child(0);
    tag_destroy(child);
  }

  if (tag->parent())
    tag->parent()->removeChild(tag);

  MySqlQuery query(get());
  QString queryString = "DELETE FROM audio_work_tags WHERE tag_id = :tag_id";
  query.prepare(queryString);
  query.bindValue(":tag_id", tag->id());
  query.exec();

  queryString = "DELETE FROM tags WHERE id = :tag_id";
  query.prepare(queryString);
  query.bindValue(":tag_id", tag->id());
  query.exec();

  delete tag;
}

int DB::artist_find(const QString& name, bool create) throw(std::runtime_error) {
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

int DB::album_find(const QString& name, bool create)  throw(std::runtime_error) {
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

int DB::file_type_find(const QString& name, bool create) throw(std::runtime_error) {
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

