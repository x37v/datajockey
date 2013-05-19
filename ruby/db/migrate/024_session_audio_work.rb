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

class AudioWorkHistory < ActiveRecord::Base
end

class AudioWork < ActiveRecord::Base
  has_many :audio_work_histories
end

class SessionAudioWork < ActiveRecord::Migration
  def self.up
    add_column :audio_works, :last_session_id, :integer
    add_column :audio_works, :last_played_at, :datetime

    AudioWork.reset_column_information
  
    AudioWorkHistory.select("*").group(:audio_work_id).order("session_id DESC").each do |h|
      w = h.audio_work
      w.update_attributes(
        :last_session_id => h.session_id,
        :last_played_at => h.played_at
      )
    end
  end

  def self.down
    remove_column :audio_works, :last_session_id
    remove_column :audio_works, :last_played_at
  end
end

