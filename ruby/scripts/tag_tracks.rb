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
require 'irb'

#connect to the database
Datajockey::connect

include Datajockey

tag_name = 'halloween'

t = Tag.where(:name => tag_name)
unless t
  c = TagClass.where(:name => 'mix').first
  t = Tag.create(:name => tag_name, :tag_class => c)
  t.save
end

#t2 = Tag.where(:name => 'aug282012').first

ARGV.each do |f|
  next if f =~ /\#/
  f = f.sub("media_drive", "x")
  w = AudioWork.where(:audio_file_location => f).first
  if w
    #unless w.tags.include?(t2) or w.tags.include?(t)
    unless w.tags.include?(t)
      w.tags << t
      w.save
    end
  else
    puts "#{f} not found in db"
  end
end
