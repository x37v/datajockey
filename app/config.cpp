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
#include <QFileInfo>
#include <QFile>
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
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
      return; //XXX error
    QTextStream in(&file);
    YAML::Node root = YAML::Load(in.readAll().toStdString());
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
      const YAML::Node& osc = root["osc"];
      try {
        mOscInPort = osc["in_port"].as<int>();
      } catch (...) { /* do nothing */ }

      const YAML::Node& scripts = root["script"];
      try {
        std::string s = scripts["post_startup"].as<std::string>();
        mPostStartScript = QString::fromStdString(s);
      } catch (...) { /* do nothing */ }

      const YAML::Node& osc_out = osc["out"];
      //the destinations could be a list or a map
      try {
        //first try map
        std::string address = osc_out["address"].as<std::string>();
        int port = osc_out["port"].as<int>();
        mOscDestinations << dj::OscNetAddr(QString::fromStdString(address), port);
      } catch(...) {
        try {
          for (unsigned int i = 0; i < osc_out.size(); i++) {
            std::string address = osc_out[i]["address"].as<std::string>();
            int port = osc_out[i]["port"].as<int>();
            mOscDestinations << dj::OscNetAddr(QString::fromStdString(address), port);
          }
        } catch (...) { /* do nothing */ }
      }

    } catch (...) { /* do nothing */ }

    try {
      std::string dir_name = root["annotation"]["files"].as<std::string>();
      QString qdir = QString::fromStdString(dir_name).trimmed();
      QDir dir(qdir.replace(QRegExp("^~"), QDir::homePath()));
      mAnnotationDir = dir.absolutePath();
    } catch (...) { /* do nothing */ }

    try {
      std::string file_name = root["midi_map"]["file"].as<std::string>();
      mMIDIMapFile = 
        QString::fromStdString(file_name).trimmed().replace(QRegExp("^~"), QDir::homePath());
    } catch (...) { /* do nothing */ }

    try {
      mMIDIMapAutoSave = root["midi_map"]["autosave"].as<bool>();
    } catch (...) { /* do nothing */ }

    //check out the import settings
    try {
      for (auto it = root["import"]["ignore"].begin(); it != root["import"]["ignore"].end(); it++)
        mImportIgnores << QString::fromStdString(it->as<std::string>());
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
    if (db[element.toStdString()]) {
      result = QString::fromStdString(db[element.toStdString()].as<std::string>());
      return true;
    }
  } catch (...) { /* do nothing */ }
  return false;
}

QString Configuration::db_adapter() const { return mDBAdapter; }
QString Configuration::db_name() const { return mDBName; }
QString Configuration::db_password() const { return mDBPassword; }
QString Configuration::db_username() const { return mDBUserName; }
QString Configuration::db_host() const { return mDBHost; }
int Configuration::db_port() const { return mDBPort; }

QString Configuration::eq_uri() const { return mEqPluginURI; }
QString Configuration::eq_port_symbol_low() const { return mEqPluginLowSymbol; }
QString Configuration::eq_port_symbol_mid() const { return mEqPluginMidSymbol; }
QString Configuration::eq_port_symbol_high() const { return mEqPluginHighSymbol; }
QString Configuration::eq_plugin_preset_file() const { return mEqPluginPresetFile; }

QString Configuration::post_start_script() {
  return mPostStartScript;
}

unsigned int Configuration::osc_in_port(){ return mOscInPort; }
const QList<dj::OscNetAddr> Configuration::osc_destinations() const { return mOscDestinations; }
QString Configuration::annotation_dir() { return mAnnotationDir; }

QString Configuration::midi_mapping_file() { return mMIDIMapFile; }
bool Configuration::midi_mapping_auto_save() { return mMIDIMapAutoSave; }

const QStringList& Configuration::import_ignores() const {
  return mImportIgnores;
}

double Configuration::import_max_seconds() const {
  return mImportMaxSeconds;
}

void Configuration::restore_defaults() {
  mDBUserName = "user";
  mDBPassword = "";
  mDBAdapter = "QSQLITE";
  mDBName = DEFAULT_DB_NAME;
  mDBHost = "localhost";
  mDBPort = -1;

  mPostStartScript.clear();

  mAnnotationDir = DEFAULT_ANNOTATION_DIR;

  mOscInPort = DEFAULT_OSC_PORT;

  mMIDIMapFile = DEFAULT_MIDI_MAPPING_FILE;
  mMIDIMapAutoSave = DEFAULT_MIDI_AUTO_SAVE;

  mValidFile = false;
}

