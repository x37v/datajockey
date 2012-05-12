#ifndef DATAJOCKEY_DB_HPP
#define DATAJOCKEY_DB_HPP

#include <QSqlDatabase>
#include <QString>
#include <QMap>
#include <QVariant>
#include <stdexcept>

namespace DataJockey {
   namespace Model {
      namespace db {
         void setup(
               QString type, 
               QString name, 
               QString username = "",
               QString password = "", 
               int port = -1, 
               QString host = QString("localhost")) throw(std::runtime_error);

         QSqlDatabase get();
         void close();

			bool find_locations_by_id(
					int work_id,
					QString& audio_file_loc,
					QString& annotation_file_loc);
			bool find_artist_and_title_by_id(
					int work_id,
					QString& artist_name,
					QString& work_title);

			//XXX use active record to get query strings!!!

			namespace work {
				int create(
						const QMap<QString, QVariant>& attributes,
						const QString& audio_file_location,
						const QString& annotation_file_location
						);
				void descriptor_create_or_update(
						int work_id,
						const QString& descriptor_type,
						float value);
				void tag(
						int work_id,
						const QString& tag_class,
						const QString& tag_value);
			}

			namespace audio_file {
				int create(const QString& location);
				QString location_by_work_id(int work_id);
			}

			namespace annotation_file {
				int create(const QString& location);
				QString location_by_work_id(int work_id);
			}

			namespace artist {
				int find(const QString& name, bool create = false);
			}

			namespace album {
				int find(const QString& name, bool create = false);
			}
      }
   }
}

#endif
