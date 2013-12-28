#ifndef DATAJOCKEY_DB_HPP
#define DATAJOCKEY_DB_HPP

#include <QSqlDatabase>
#include <QDateTime>
#include <QString>
#include <QHash>
#include <QVariant>
#include <stdexcept>

class DB : public QObject {
  Q_OBJECT
  public:
    DB(
        QString type, 
        QString name_or_location, 
        QString username = "",
        QString password = "", 
        int port = -1, 
        QString host = QString("localhost"),
        QObject * parent = NULL
      ) throw(std::runtime_error);

    virtual ~DB();

    QSqlDatabase get();
    void close();

    bool find_locations_by_id(
        int work_id,
        QString& audio_file_loc,
        QString& annotation_file_loc) throw(std::runtime_error);
    bool find_artist_and_title_by_id(
        int work_id,
        QString& artist_name,
        QString& work_title) throw(std::runtime_error);

    void format_string_by_id(int work_id, QString& info) throw(std::runtime_error);

    QString work_table_query(const QString where_clause = QString()) throw(std::runtime_error);

    int work_table_column(QString id_name);

    int work_create(
        const QHash<QString, QVariant>& attributes,
        const QString& audio_file_location
        ) throw(std::runtime_error);
    int work_find_by_audio_file_location(
        const QString& audio_file_location) throw(std::runtime_error);
    void work_update_attribute(
        int work_id,
        const QString& name,
        const QVariant& value) throw(std::runtime_error);
    void work_descriptor_create_or_update(
        int work_id,
        QString descriptor_type_name,
        double value) throw(std::runtime_error);
    void work_tag(
        int work_id,
        const QString& tag_class,
        const QString& tag_value) throw(std::runtime_error);
    void work_set_album(int work_id, int album_id, int track_num)  throw(std::runtime_error);

    int current_session();

    int tag_find_class(const QString& name) throw(std::runtime_error);
    int tag_find(const QString& name, int tag_class_id = -1) throw(std::runtime_error);

    int artist_find(const QString& name, bool create = false) throw(std::runtime_error);

    int album_find(const QString& name, bool create = false) throw(std::runtime_error);

    int file_type_find(const QString& name, bool create = false) throw(std::runtime_error);

  public slots:
    //indicate that this work has been played in this session, at time given
    void work_set_played(int work_id, QDateTime time);

  private:
    QSqlDatabase mDB;
};

#endif
