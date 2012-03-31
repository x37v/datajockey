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

class AddIndicies < ActiveRecord::Migration
  def self.up
    add_index("artist_audio_works", ["artist_id"])
    add_index("artist_audio_works", ["audio_work_id"])
    add_index("album_audio_works", ["audio_work_id"])
    add_index("album_audio_works", ["album_id"])
    add_index("descriptors", ["audio_work_id"])
    add_index("audio_works", ["audio_file_id"])
    add_index("descriptors", ["descriptor_type_id"])
    add_index("descriptors", ["audio_work_id", "descriptor_type_id"])
    #add_index("descriptors", ["audio_work_id", "descriptor_type_id"])
  end

  def self.down
    remove_index("artist_audio_works", "artist_id") 
    remove_index("artist_audio_works", "audio_work_id") 
    remove_index("album_audio_works", "audio_work_id") 
    remove_index("album_audio_works", "album_id") 
    remove_index("descriptors", "audio_work_id") 
    remove_index("audio_works", "audio_file_id")
    #XXX THIS DOESN'T WORK!
    remove_index("descriptors", "descriptor_type_id") 
    remove_index("descriptors", ["audio_work_id", "descriptor_type_id"])
  end
end
