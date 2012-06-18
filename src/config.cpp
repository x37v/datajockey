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
#include <fstream>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QRegExp>
#include <QDebug>

using namespace dj;

Configuration * Configuration::cInstance = NULL;

Configuration::Configuration(){ 
   restore_defaults();
   mValidFile = false;
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

void Configuration::load_default() {
   std::vector<QString> search_paths;
   QString homeConfig(QDir::homePath());
   homeConfig.append("/.datajockey/config.yaml");

   search_paths.push_back(homeConfig);
   search_paths.push_back("./config.yaml");
   //search_paths.push_back("/usr/local/share/datajockey/config.yaml");
   //search_paths.push_back("/usr/share/datajockey/config.yaml");

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
   restore_defaults();
}

void Configuration::load_file(const QString& path) throw(std::runtime_error) {
   restore_defaults();
   try {
      std::ifstream fin(path.toStdString().c_str());
      YAML::Parser p(fin);
      YAML::Node root;
      p.GetNextDocument(root);
      mValidFile = true;
      mFile = path;

      //fill in the db entries
      QString adapter;
      if (db_get(root, "adapter", adapter)) {
         if (adapter.contains("mysql"))
            mDBAdapter = "QMYSQL";
         else if (adapter.contains("sqlite"))
            mDBAdapter = "QSQLITE";
         else
            qWarning() << "config file " << path << " contains invalid adapter type: " << adapter;
      }
      db_get(root, "database", mDBName);
      db_get(root, "username", mDBUserName);
      db_get(root, "password", mDBPassword);

      //fill in the rest
      try {
         root["osc_port"] >> mOscPort;
      } catch (...) { /* do nothing */ }

      try {
         std::string dir_name;
         root["annotation"]["files"] >> dir_name;
         QString qdir = QString::fromStdString(dir_name).trimmed();
         QDir dir(qdir.replace(QRegExp("^~"), QDir::homePath()));
         mAnnotationDir = dir.absolutePath();
      } catch (...) { /* do nothing */ }

      try {
         std::string file_name;
         root["midi_map"]["file"] >> file_name;
         mMIDIMapFile = 
            QString::fromStdString(file_name).trimmed().replace(QRegExp("^~"), QDir::homePath());
      } catch (...) { /* do nothing */ }

      try {
         root["midi_map"]["autosave"] >> mMIDIMapAutoSave;
      } catch (...) { /* do nothing */ }

   } catch (...){
      mValidFile = false;
      std::string str("error reading config file: ");
      str.append(path.toStdString());
      throw std::runtime_error(str);
   }
}

QString Configuration::file(){
   if(!instance()->mValidFile)
      return QString();
   else
      return instance()->mFile;
}

bool Configuration::valid_file(){
   return mValidFile;
}

bool Configuration::db_get(YAML::Node& doc, QString element, QString &result) {
   try {
      const YAML::Node& db = doc["database"];
      if(const YAML::Node *n = db.FindValue(element.toStdString())) {
         std::string r;
         *n >> r;
         result = QString::fromStdString(r);
         return true;
      }
   } catch (...) { /* do nothing */ }
   return false;
}

QString Configuration::db_adapter() { return mDBAdapter; }
QString Configuration::db_name() { return mDBName; }
QString Configuration::db_password() { return mDBPassword; }
QString Configuration::db_username() { return mDBUserName; }

unsigned int Configuration::osc_port(){ return mOscPort; }
QString Configuration::annotation_dir() { return mAnnotationDir; }

QString Configuration::midi_mapping_file() { return mMIDIMapFile; }
bool Configuration::midi_mapping_auto_save() { return mMIDIMapAutoSave; }

void Configuration::restore_defaults() {
   mDBUserName = "user";
   mDBPassword = "";
   mDBAdapter = "QSQLITE";
   mDBName = DEFAULT_DB_NAME;

   mAnnotationDir = DEFAULT_ANNOTATION_DIR;

   mOscPort = DEFAULT_OSC_PORT;

   mMIDIMapFile = DEFAULT_MIDI_MAPPING_FILE;
   mMIDIMapAutoSave = DEFAULT_MIDI_AUTO_SAVE;

   mValidFile = false;
}

