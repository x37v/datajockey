#ifndef DATAJOCKEY_DB_HPP
#define DATAJOCKEY_DB_HPP

#include <QSqlDatabase>
#include <QString>

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
      }
   }
}

#endif
