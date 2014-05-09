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

AudioWorkJump.all.each do |j|
  w = j.audio_work
  annotation = YAML.load(File.read(w.annotation_file_location))
  beats = annotation["beat_locations"]
  next unless beats
  beats = beats["frames"]
  raise "no beats" unless beats and beats.size > 1
  data = YAML::load(j.data)

  data.each do |e|
    f = e["frame_start"].to_i
    e.delete("frame_start")
    e.delete("frame_end")
    
    b = beats.find_index(f)
    e["location_type"] = "beat"
    e["start"] = b
    e["end"] = b
  end
  puts data
  j.data = data.to_s
  j.save!
end

puts "DONE!"
