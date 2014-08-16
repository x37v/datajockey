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
require_relative 'getsamplerate'

#connect to the database
Datajockey::connect

include Datajockey

t = Tag.find_by_name("half_beat")

t.audio_works.each do |w|
  annotation = YAML.load(File.read(w.annotation_file_location))
  beats = annotation["beat_locations"]
  next unless beats
  beats = beats["frames"]
  next unless beats and beats.size > 1
  dist = beats[0..-2].zip(beats[1..-1]).collect { |t0, t1| t1 - t0 }.sort
  median = dist[dist.size / 2]
  if dist.size % 2 == 0
    median = (median + dist[dist.size / 2 + 1]).to_f / 2
  end
  off = (median.to_f / 2).to_i
  beats = beats.collect { |b| b + off }
  annotation["beat_locations"]["frames"] = beats
  puts "#{w.name} #{median} #{off}"
  puts w.annotation_file_location
  AudioWorkTag.where(:tag_id => t.id, :audio_work_id => w.id).each do |tw|
    tw.destroy
  end
  File.open(w.annotation_file_location, "w") { |f|  f.print annotation.to_yaml }
end

puts "DONE!"
