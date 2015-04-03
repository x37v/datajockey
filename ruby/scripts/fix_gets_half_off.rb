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

t = Tag.find_by_name("gets_off_half")

#tag a work with gets_off_half and put marker 4 just before the start of the transition up zone, and marker 5 when it gets 1/2 off and steady
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

  annotation = YAML.load(File.read(w.annotation_file_location))
  beats = annotation["beat_locations"]
  next unless beats
  beats = beats["frames"]
  next unless beats and beats.size > 1

  good_start = beats[0..range[0]]
  end_off_half = beats[range[1]..-1]

  dist = good_start[0..-2].zip(good_start[1..-1]).collect { |t0, t1| t1 - t0 }.sort
  median = dist[dist.size / 2]
  if dist.size % 2 == 0
    median = (median + dist[dist.size / 2 + 1]).to_f / 2
  end

  #offset end
  off = (median.to_f / 2).to_i
  end_off_half = end_off_half.collect { |b| b + off }

  puts "recalc for #{good_start[-1]} to #{end_off_half[0]}"

  count = (end_off_half[0] - good_start[-1]) / median
  puts "num beats off #{count}"

  (count - 1).round.times do
    v = good_start[-1] + median
    good_start << v.to_i
  end
  puts "#{end_off_half[0] - good_start[-1]} median: #{median}"
  beats = good_start + end_off_half

  annotation["beat_locations"]["frames"] = beats
  puts "#{w.name} #{median} done"
  puts w.annotation_file_location

  File.open(w.annotation_file_location, "w") { |f|  f.print annotation.to_yaml }
end

puts "DONE!"
