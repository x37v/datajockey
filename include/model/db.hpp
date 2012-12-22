#ifndef DATAJOCKEY_DB_HPP
#define DATAJOCKEY_DB_HPP

#include <QSqlDatabase>
#include <QString>
#include <QHash>
#include <QVariant>
#include <stdexcept>

namespace dj {
  namespace model {
    namespace db {
      void setup(
          QString type, 
          QString name_or_location, 
          QString username = "",
          QString password = "", 
          int port = -1, 
          QString host = QString("localhost")) throw(std::runtime_error);

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

      namespace work {
        QString filtered_table_query(const QString where_clause = QString(), const QString session_clause = QString()) throw(std::runtime_error);
        QString filtered_table(const QString where_clause = QString()) throw(std::runtime_error);
        void filtered_update(const QString& table_name, const QString where_clause = QString()) throw(std::runtime_error);
        QString filtered_table_by_tags(QList<int> tag_ids) throw(std::runtime_error);
        QString filtered_table_by_tag(int tag_id) throw(std::runtime_error);
        QString filtered_table_by_tag(QString tag, QString tag_class = QString()) throw(std::runtime_error);

        int temp_table_id_column(QString id_name);
        void temp_table_id_columns(QList<int>& ids);

        int create(
            const QHash<QString, QVariant>& attributes,
            const QString& audio_file_location
            ) throw(std::runtime_error);
        int find_by_audio_file_location(
            const QString& audio_file_location) throw(std::runtime_error);
        void update_attribute(
            int work_id,
            const QString& name,
            const QVariant& value) throw(std::runtime_error);
        void descriptor_create_or_update(
            int work_id,
            const QString& descriptor_type_name,
            double value) throw(std::runtime_error);
        void tag(
            int work_id,
            const QString& tag_class,
            const QString& tag_value) throw(std::runtime_error);

        //indicate that this work has been played in this session, at time given
        void set_played(int work_id, int session_id, QDateTime time);

        int current_session();
      }

      namespace tag {
        int find_class(const QString& name) throw(std::runtime_error);
        int find(const QString& name, int tag_class_id = -1) throw(std::runtime_error);
      }

      namespace artist {
        int find(const QString& name, bool create = false) throw(std::runtime_error);
      }

      namespace album {
        int find(const QString& name, bool create = false) throw(std::runtime_error);
        void add_work(int album_id, int work_id, int track_num) throw(std::runtime_error);
      }

      namespace file_type {
        int find(const QString& name, bool create = false) throw(std::runtime_error);
      }
    }
  }
}

#endif
