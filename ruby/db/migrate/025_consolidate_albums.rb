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

class AlbumAudioWork < ActiveRecord::Base
  belongs_to :audio_work
end

class AudioWork < ActiveRecord::Base
end

class ConsolidateAlbums < ActiveRecord::Migration
  def self.up
    add_column :audio_works, :album_id, :integer
    add_column :audio_works, :album_track, :integer

    AudioWork.reset_column_information
    AlbumAudioWork.all.each do |a|
      w = a.audio_work
      next unless w
      w.update_attributes(
        :album_id => a.album_id,
        :album_track => a.track
      )
    end
    
    drop_table :album_audio_works
  end

  def self.down
    remove_column :audio_works, :album_id
    remove_column :audio_works, :album_track

    create_table :album_audio_works do |t|
      t.column :album_id, :integer
      t.column :audio_work_id, :integer
      t.column :track, :integer
    end

    AudioWork.all.each do |w|
      if w.album_id
        a = AlbumAudioWork.create(:audio_work_id => w.id,
                              :album_id => w.album_id,
                              :track => w.album_track)
        a.save!
      end
    end
  end
end

