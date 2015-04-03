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
require 'pathname'
require_relative 'getsamplerate'

#connect to the database
Datajockey::connect

include Datajockey

t = Tag.find_by_name("middle_off")

#tag a work with middle_off and put marker 4 just before the start of the fucked up zone, and marker 5 just after
#NOTE markers are shown as + 1 in GUI, so in GUI it'll look like 5 and 6

t.audio_works.each do |w|
  puts "#{w.name} #{w.artist.name}"
  range = [w.jumps[4], w.jumps[5]]
  if range[0] and range[1]
    range.collect! { |r| r[:start] }
    puts "got shit #{range[0]} #{range[1]}"
  else
    next
  end

  #copy the shit
  puts w.annotation_file_location
  f = Pathname.new(w.annotation_file_location)
  FileUtils.cp(w.annotation_file_location, File.join("/tmp/", f.basename))

  annotation = YAML.load(File.read(w.annotation_file_location))
  beats = annotation["beat_locations"]
  next unless beats
  beats = beats["frames"]
  next unless beats and beats.size > 1

  #find the tempo
  n = 40
  dist = beats[range[0] - n ..range[0] - 1].zip(beats[range[0] - n + 1..range[0]]).collect { |t0, t1| t1 - t0 }.sort
  median = dist[dist.size / 2]
  if dist.size % 2 == 0
    median = (median + dist[dist.size / 2 + 1]).to_f / 2
  end
  puts median

  good_start = beats[0..range[0]]
  good_end = beats[range[1]..-1]

  puts "recalc for #{good_start[-1]} to #{good_end[0]}"

  count = (good_end[0] - good_start[-1]) / median
  puts "num beats off #{count}"

  (count - 1).round.times do
    v = good_start[-1] + median
    good_start << v.to_i
  end
  puts "#{good_end[0] - good_start[-1]} median: #{median}"
  beats = good_start + good_end

  #smooth between points
  10.times do |t|
    (range[0] - 4 .. range[1] + 4).each do |i|
      n = ((beats[i + 1] - beats[i - 1]) / 2 + beats[i - 1]).to_i
      beats[i] = n
    end
    (range[1] + 4).downto(range[0] - 4).each do |i|
      n = ((beats[i + 1] - beats[i - 1]) / 2 + beats[i - 1]).to_i
      beats[i] = n
    end
  end

  annotation["beat_locations"]["frames"] = beats
  File.open(w.annotation_file_location, "w") { |f|  f.print annotation.to_yaml }
end

puts "DONE!"
