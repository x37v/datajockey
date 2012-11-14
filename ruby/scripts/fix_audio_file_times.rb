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

#update time info for all the files we can get
AudioWork.all.each do |w|
  f = w.audio_file_location
  s = 0

  #find the time from the file
  if f =~ /\.mp3/
    s = `mp3info -p "%S" "#{f}"`.to_i
  else
    v = `sndfile-info "#{f}" | grep Duration`.split(':')
    if v.length != 4
      puts "problem grabbin sound info for #{f}"
      next
    end
    s = ((v[1].to_i * 60 + v[2].to_i) * 60 + v[3].to_f).to_i
  end

  if s == 0
    puts "problem with #{f}"
  else
    w.update_attribute(:audio_file_seconds, s)
  end
end

