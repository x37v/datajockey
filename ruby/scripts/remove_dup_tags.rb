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

AudioWorkTag.group(:tag_id, :audio_work_id).count.each do |g, c|
  next if c < 2
  AudioWorkTag.where(:tag_id => g[0], :audio_work_id => g[1]).to_a[1..-1].each do |t|
    t.destroy
  end
end
