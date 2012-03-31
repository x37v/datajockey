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

class CreateFileIdTable < ActiveRecord::Migration
  def self.up
		create_table :audio_files do |t|
      t.column :audio_file_type_id, :integer
      t.column :location, :string, :limit => 255
      t.column :milliseconds, :integer
      t.column :channels, :integer, :default => 2
      t.column :created_at, :datetime
      t.column :updated_at, :datetime
    end

		create_table :audio_file_types do |t|
      t.column :name, :string
    end

    AudioFileType.create :name => 'mp3'
    AudioFileType.create :name => 'ogg'
    AudioFileType.create :name => 'flac'
    AudioFileType.create :name => 'wav'
    AudioFileType.create :name => 'aiff'
  end

  def self.down
    drop_table :audio_files
    drop_table :audio_file_types
  end
end
