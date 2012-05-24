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

class AudioFile < ActiveRecord::Base
  has_one :audio_work
end

class Artist < ActiveRecord::Base
  has_many :artist_audio_works, :dependent => :destroy
  has_many :audio_works, :through => :artist_audio_works
end

class ArtistAudioWork < ActiveRecord::Base
  belongs_to :artist
  belongs_to :audio_work
  belongs_to :artist_role
end

class AnnotationFile < ActiveRecord::Base
  belongs_to :audio_work
end

class AudioWork < ActiveRecord::Base
  belongs_to :audio_file
  has_many :artist_audio_works, :dependent => :destroy
  has_one :annotation_file, :dependent => :destroy
end

class ConsolidateLocationsArtistAudioWorks < ActiveRecord::Migration
  def self.up
    #audio file
    add_column :audio_works, :audio_file_type_id, :integer
    add_column :audio_works, :audio_file_location, :string, :limit => 255
    add_column :audio_works, :audio_file_milliseconds, :integer
    add_column :audio_works, :audio_file_channels, :integer, :default => 2

    #annotation file
    add_column :audio_works, :annotation_file_location, :string, :limit => 255

    #artist
    add_column :audio_works, :artist_id, :integer

    AudioWork.reset_column_information
    AudioWork.all.each do |w|
      #artist
      a = w.artist_audio_works
      if a.first
        w.artist_id = a.first.artist_id
        a.first.destroy
      end

      #audio file
      f = w.audio_file
      if f
        w.update_attributes(
          :audio_file_type_id => f.audio_file_type_id,
          :audio_file_location => f.location,
          :audio_file_milliseconds => f.milliseconds,
          :audio_file_channels => f.channels
        )
      end

      #annotation file
      l = w.annotation_file
      if l
        w.update_attribute(:annotation_file_location, l.location)
      end
    end

    drop_table :audio_files
    drop_table :annotation_files
  end

  def self.down
		create_table :audio_files do |t|
      t.column :audio_file_type_id, :integer
      t.column :location, :string, :limit => 255
      t.column :milliseconds, :integer
      t.column :channels, :integer, :default => 2
      t.column :created_at, :datetime
      t.column :updated_at, :datetime
    end

    create_table :annotation_files do |t|
      t.column :audio_work_id, :integer
      t.column :location, :string, :limit => 255
      t.column :created_at, :datetime
      t.column :updated_at, :datetime
    end

    AudioWork.all.each do |w|
      AudioFile.create(
          :audio_file_type_id => w.audio_file_type_id,
          :location => w.audio_file_location,
          :milliseconds => w.audio_file_milliseconds,
          :channels => w.audio_file_channels
      )
      AnnotationFile.create(:location => w.annotation_file_location)

      #XXX deal with artists
    end

    remove_column :audio_works, :audio_file_type_id
    remove_column :audio_works, :audio_file_location
    remove_column :audio_works, :audio_file_milliseconds
    remove_column :audio_works, :audio_file_channels

    remove_column :audio_works, :annotation_file_location

    remove_column :audio_works, :artist_id
  end
end
