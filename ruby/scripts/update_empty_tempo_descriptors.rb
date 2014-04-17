#!/usr/bin/env ruby

=begin
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

$: << ".."

require 'datajockey/base'
require 'datajockey/db'
require 'yaml'

#connect to the database
Datajockey::connect

include Datajockey

AudioWork.where('descriptor_tempo_median IS NULL AND annotation_file_location IS NOT NULL').each do |w|
  annotation = YAML.load(File.read(w.annotation_file_location))
  beats = annotation["beat_locations"]
  next unless beats
  beats = beats["frames"]
  next unless beats and beats.size > 1
  #XXX actually do it!
end
