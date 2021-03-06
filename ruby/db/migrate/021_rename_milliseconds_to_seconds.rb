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

class RenameMillisecondsToSeconds < ActiveRecord::Migration
  def self.up
    rename_column :audio_works, :audio_file_milliseconds, :audio_file_seconds
  end

  def self.down
    rename_column :audio_works, :audio_file_seconds, :audio_file_milliseconds
  end
end
