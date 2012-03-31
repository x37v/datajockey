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

class CreateArtistAudioWorks < ActiveRecord::Migration
  def self.up
    create_table :artist_roles do |t|
      t.column :name, :string
    end

    ArtistRole.create :name => 'composer'
    ArtistRole.create :name => 'performer'

    create_table :artist_audio_works do |t|
      t.column :artist_id, :integer
      t.column :audio_work_id, :integer
      t.column :artist_role_id, :integer
    end
  end

  def self.down
    drop_table :artist_audio_works
    drop_table :artist_roles
  end
end
