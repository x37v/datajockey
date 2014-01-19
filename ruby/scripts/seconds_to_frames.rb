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
require 'shellwords'
require 'mp3info'

#connect to the database
Datajockey::connect
include Datajockey

problems = File.open("problems.txt", "w")

#ids = File.readlines("problems.txt").collect do |l|
#  l.split(/\s/).first.to_i
#end

#AudioWork.find(ids).each do |w|
AudioWork.all.each do |w|
  a = w.annotation_file_location
  f = w.audio_file_location
  puts a

  begin
    sr = nil
    if f =~ /flac/
      cmd = "sndfile-info"
    elsif f =~ /mp3/
      Mp3Info.open(f) do |info|
        sr = info.samplerate
      end
    else
      cmd = "exiftool"
    end
    unless sr
      sr = `#{cmd} #{Shellwords.escape(f)} | grep "Sample Rate"`.chomp.split(" ").last.to_f
    end
    if sr == 0
      puts "#{w.id} #{w.name} #{f} sample rate? #{sr}"
      problems.puts "#{w.id} #{f} #{a} sample_rate=0"
    end

    yaml = YAML.load(File.read(File.expand_path(a)))
    seconds = yaml['beat_locations']['time_points']
    unless seconds and seconds.length
      next
    end
    frames = seconds.collect do |s|
      (s * sr).to_i
    end
    yaml['beat_locations'].delete('time_points')
    yaml['beat_locations']['frames'] = frames
    File.open(a, "w") { |f| f.puts yaml.to_yaml }
  rescue => e
    puts "problem #{e}"
    problems.puts "#{w.id} #{f} #{a} #{e}"
  end
end

