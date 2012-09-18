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

d = DescriptorType.where(:name => 'tempo median').first
Descriptor.where(:descriptor_type_id => d.id, :value => (108..132)).each do |d|
  w = d.audio_work
  if w and w.artist and w.album
    puts "#{d.value}, #{w.artist.name}, #{w.name}, #{w.album.name}, #{w.audio_file_location}, #{w.annotation_file_location}"
  end
end
