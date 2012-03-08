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
               QString host = QString("localhost"));
         QSqlDatabase get();
         void close();
      }
   }
}

#endif
