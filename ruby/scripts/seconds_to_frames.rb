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

AudioWork.all.each do |w|
  a = w.annotation_file_location
  f = w.audio_file_location
  if f =~ /flac/
    sr = `sndfile-info #{f} | grep "Sample Rate"`.chomp.split(" ").last.to_f
  elsif f =~ /mp3/
    sr = `exiftool #{f} | grep "Sample Rate"`.chomp.split(" ").last.to_f
  end
  if sr < 44100 or sr > 48000
    puts "#{w.id} #{w.name} #{f} sample rate? #{sr}"
    exit
  end

  yaml = YAML.load(File.read(File.expand_path(a)))
  seconds = yaml['beat_locations']['time_points']
  unless seconds and seconds.length
    puts "skipping #{a}"
    next
  end
  frames = seconds.collect do |s|
    (s * sr).to_i
  end
  yaml['beat_locations'].delete('time_points')
  yaml['beat_locations']['frames'] = frames
  File.open(a, "w") { |f| f.puts yaml.to_yaml }
  puts a
end
