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
#define DEFAULT_MIDI_MAPPING_FILE (QDir::homePath() + "/.datajockey/midimap.yaml")
#define DEFAULT_MIDI_AUTO_SAVE true


#include <stdexcept>
#include <QString>
#include <QStringList>
#include <QPair>
#include <yaml-cpp/yaml.h>
#include <array>
#include "defines.hpp"

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
      QString db_adapter() const;
      QString db_name() const;
      QString db_username() const;
      QString db_password() const;
      QString db_host() const;
      int db_port() const;

      QString eq_plugin_uuid() const;
      QString eq_uri() const;
      QString eq_port_symbol(dj::eq_band_t band) const;
      //zero for no db scaling
      float eq_band_db_scale(dj::eq_band_t band) const;
      QString plugin_preset_file(QString pluginUUID);

      //if populated, run this after startup
      QString post_start_script();

      unsigned int osc_in_port();
      const QList<dj::OscNetAddr> osc_destinations() const;

      QString annotation_dir();
      QString midi_mapping_file();
      bool midi_mapping_auto_save();

      const QStringList& import_ignores() const;
      double import_max_seconds() const;
    private:
      bool db_get(YAML::Node& doc, QString entry, QString &result);
      QString mFile;

      QString mDBAdapter;
      QString mDBName;
      QString mDBUserName;
      QString mDBPassword;
      QString mDBHost;
      int mDBPort;

      QString mEqPluginURI = "http://plugin.org.uk/swh-plugins/dj_eq";
      std::array<QString, 3> mEqPluginSymbol = std::array<QString,3>{"lo", "mid", "hi"};
      std::array<float, 3> mEqPluginDBScale = std::array<float,3>{0.0f, 0.0f, 0.0f};
      QString mEqPluginPresetFile = "";

      /*
      QString mEqPluginURI = "http://calf.sourceforge.net/plugins/eq5";
      std::array<QString, 3> mEqPluginSymbol = std::array<QString,3>{"ls_level", "p2_level", "hs_level"};
      std::array<float, 3> mEqPluginDBScale = std::array<float,3>{36.0f, 36.0f, 36.0f};
      QString mEqPluginPresetFile = "/home/alex/lv2presets/calfeq5_djing_maybe.lv2/calfeq5_djing_maybe.ttl";
      */

      QString mPostStartScript;

      unsigned int mOscInPort;
      QList<dj::OscNetAddr> mOscDestinations;

      QString mAnnotationDir;

      QString mMIDIMapFile;
      bool mMIDIMapAutoSave;

      QStringList mImportIgnores;

      double mImportMaxSeconds = 60.0 * 20.0;

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
