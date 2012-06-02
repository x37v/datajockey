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

#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <QFileInfo>
#include <QDir>

using namespace dj;

Configuration * Configuration::cInstance = NULL;

Configuration::Configuration(){ 
	mValid = false;
}

Configuration::Configuration(const Configuration&){ }
//Configuration& Configuration::operator=(const Configuration&){ }
Configuration::~Configuration(){ } 

Configuration * Configuration::instance(){
	if(!cInstance)
		cInstance = new Configuration();
	return cInstance;
}

/*
void Configuration::load(QString yaml_data){
	Configuration * self = Configuration::instance();
	return false;
}
*/

//search for (in order):
//~/.datajockey/config.yaml
//./config.yaml
///usr/local/share/datajockey/config.yaml
///usr/share/datajockey/config.yaml

void Configuration::load_default() throw(std::runtime_error) {
	std::vector<QString> search_paths;
	QString homeConfig(QDir::homePath());
	homeConfig.append("/.datajockey/config.yaml");

	search_paths.push_back(homeConfig);
	search_paths.push_back("./config.yaml");
	search_paths.push_back("/usr/local/share/datajockey/config.yaml");
	search_paths.push_back("/usr/share/datajockey/config.yaml");

	for(std::vector<QString>::iterator it = search_paths.begin(); it != search_paths.end(); it++){
		try {
         QFileInfo file_info(*it);
         if (file_info.exists() && file_info.isFile()) {
				load_file(*it);
            mFile = *it;
				return;
			} 
		} catch (...){
			//we don't actually do anything because we're going to try another location
			std::cerr << it->toStdString() << " is not a valid configuration file, trying the next location" << std::endl;
		}
	}
	//if we're here then we didn't find or successfully load a config file
   std::string str("Cannot find a valid configuration file to load in one of the standard locations:");
	for(std::vector<QString>::iterator it = search_paths.begin(); it != search_paths.end(); it++){
		str.append("\n");
		str.append(it->toStdString());
	}
	throw std::runtime_error(str);
}

void Configuration::load_file(const QString& path) throw(std::runtime_error) {
	try {
      std::ifstream fin(path.toStdString().c_str());
		YAML::Parser p(fin);
		p.GetNextDocument(mRoot);
		mValid = true;
      mFile = path;
	} catch (...){
		mValid = false;
      std::string str("error reading config file: ");
		str.append(path.toStdString());
		throw std::runtime_error(str);
	}
}

QString Configuration::file(){
   if(!instance()->mValid)
      return QString();
   else
      return instance()->mFile;
}

bool Configuration::valid(){
	return mValid;
}

bool Configuration::db_get(QString element, QString &result) throw(std::runtime_error){
	if(!mValid)
		throw std::runtime_error("trying to access data from an invalid configuration");
	try {
      const YAML::Node& db = mRoot["database"];
      if(const YAML::Node *n = db.FindValue(element.toStdString())) {
         std::string r;
         *n >> r;
         result = QString::fromStdString(r);
         return true;
      } else
         return false;
	} catch(std::exception &e){
		throw;
	} catch(...){
      std::string str("unable to access database information for: ");
		str.append(element.toStdString());
		throw std::runtime_error(str);
	}
}

QString Configuration::db_adapter() throw(std::runtime_error){
	QString adapter;
	if(!db_get("adapter", adapter))
		throw std::runtime_error("could not find database adapter");
	else if (adapter.contains("mysql"))
		return QString("QMYSQL");
	else if (adapter.contains("sqlite"))
		return QString("QSQLITE");
	else
		throw std::runtime_error("invalid database adapter");
}

QString Configuration::db_name() throw(std::runtime_error){
	QString name;
	if(!db_get("database", name))
		throw std::runtime_error("could not get database name");
	else
		return name;
}

QString Configuration::db_password() {
	//we don't need to have a password entry
	try {
		QString pass;
		if(db_get("password", pass))
			return pass;
	} catch (...) { /* don't do anything */}
	return QString("");
}

unsigned int Configuration::osc_port(){
	unsigned int port = DEFAULT_OSC_PORT;
   try {
      mRoot["osc_port"] >> port;
   } catch (...) {
      return DEFAULT_OSC_PORT;
   }
	
	return port;
}

QString Configuration::db_username() {
	//we don't need to have a username entry
	try {
		QString user;
		if(db_get("username", user))
			return user;
	} catch (...) { /* don't do anything */}
	return QString("");
}

