/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

//this is all about the configuration!
#ifndef DATAJOCKEY_CONFIGURATION_HPP
#define DATAJOCKEY_CONFIGURATION_HPP

#define DEFAULT_OSC_PORT 10001
#define DEFAULT_DB_NAME (QDir::homePath() + "/.datajockey/database.sqlite3")
#define DEFAULT_ANNOTATION_DIR (QDir::homePath() + "/.datajockey/annotation")


#include <stdexcept>
#include <QString>
#include <yaml-cpp/yaml.h>

namespace dj {
   class Configuration {
      public:
         static Configuration * instance();
         //this finds a config file in a default location
         void load_default();
         //void load(QString yaml_data);
         void load_file(const QString& path) throw(std::runtime_error);
         bool valid_file();

         //returns the path to the config file has been loaded
         static QString file();

         //get database data
         QString db_adapter();
         QString db_name();
         QString db_username();
         QString db_password();

         unsigned int osc_port();

         QString annotation_dir();
      private:
         bool db_get(YAML::Node& doc, QString entry, QString &result);
         QString mFile;

         QString mDBAdapter;
         QString mDBName;
         QString mDBUserName;
         QString mDBPassword;

         unsigned int mOscPort;

         QString mAnnotationDir;

      protected:
         Configuration();
         Configuration(const Configuration&);
         Configuration& operator=(const Configuration&);
         virtual ~Configuration();
      private:
         void restore_defaults();
         static Configuration * cInstance;
         bool mValidFile;
   };
}

#endif
