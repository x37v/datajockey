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

#include <stdexcept>
#include <QString>
#include <yaml-cpp/yaml.h>

namespace dj {
	class Configuration {
		public:
			static Configuration * instance();
			//this finds a config file in a default location
			void loadDefault() throw(std::runtime_error);
			//void load(QString yaml_data);
			void loadFile(const QString& path) throw(std::runtime_error);
			bool valid();

         //returns the path to the config file has been loaded
         static QString getFile();

			//get database data
			QString databaseAdapter() throw(std::runtime_error);
			QString databaseName() throw(std::runtime_error);
			QString databaseUserName();
			QString databasePassword();
			unsigned int oscPort();
		private:
			bool databaseGet(QString entry, QString &result) throw(std::runtime_error);
         QString mFile;

		protected:
			Configuration();
			Configuration(const Configuration&);
			Configuration& operator=(const Configuration&);
			virtual ~Configuration();
		private:
			static Configuration * cInstance;
			YAML::Node mRoot;
			bool mValid;
	};
}

#endif
