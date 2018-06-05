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

#connect to the database
Datajockey::connect

include Datajockey

file_location = ARGV[1]

tag_name = ARGV[0]

puts "tagging files in #{file_location} with tag '#{tag_name}' continue?"
c = STDIN.gets.chomp.to_s
exit unless c =~ /y/

puts "doing it"

files = File.readlines(file_location).collect { |f| f.chomp }

c = Tag.find_or_create_by_name_and_parent_id("mix", 0)
t = Tag.find_or_create_by_name_and_parent_id(tag_name, c.id)

#t2 = Tag.where(:name => 'aug282012').first

files.each do |f|
  next if f =~ /\#/
  f = f.sub("/media/media_drive", "/home/alex")
  w = AudioWork.where(:audio_file_location => f).first
  if w
    #unless w.tags.include?(t2) or w.tags.include?(t)
    unless w.tags.include?(t)
      AudioWorkTag.create(:tag => t, :audio_work => w)
      #w.tags << t
      #w.save
    end
  else
    puts "#{f} not found in db"
  end
end

puts "done!"
