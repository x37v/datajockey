=begin

  Copyright (c) 2008 Alex Norman.  All rights reserved.
	http://www.x37v.info/datajockey

	This file is part of Data Jockey.
	
	Data Jockey is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or (at your
	option) any later version.
	
	Data Jockey is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
	Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
=end

require 'yaml'
require 'rubygems'
require 'active_record'

module Datajockey
  #datajockey config stuff
  @@conf_file = nil
  @@conf = nil

  #tell Datajockey to use a specific config file
  def Datajockey::setConfFile(file)
    @@conf = nil
    @@conf_file = file
  end

  #find the config file, search the default locations:
  #~/.datajockey/config.yaml
  #./config.yaml
  #/usr/local/share/datajockey/config.yaml
  #/usr/share/datajockey/config.yaml
  def Datajockey::setDefaultConfFile
    @@conf = nil
    @@conf_file = nil
      config_paths = [
        File.expand_path("~/.datajockey/config.yaml"),
        "../config.yaml",
        "./config.yaml",
        "/usr/local/share/datajockey/config.yaml",
        "/usr/share/datajockey/config.yaml"
      ]
      config_paths.each do |p|
        if File.exists?(p)
          @@conf_file = p
          break
        end
      end
      if not @@conf_file
        raise "default config file cannot be found"
      end
  end
  #get the config data [yaml data]
  def Datajockey::config
    unless @@conf
      unless @@conf_file
        Datajockey::setDefaultConfFile
      end
      @@conf = YAML::load(File.open(@@conf_file))
    end
    return @@conf
  end
  #connect to the database [information gathered from the config file]
  def Datajockey::connect
    unless ActiveRecord::Base.connected?
      if Datajockey::config["database"]
        ActiveRecord::Base.establish_connection(Datajockey::config["database"])
      else
        raise "No database entry in config file, cannot establish database connection."
      end
    end
  end
end

