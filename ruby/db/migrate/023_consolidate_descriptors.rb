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

class DescriptorType < ActiveRecord::Base
end

class AudioWorks < ActiveRecord::Base
  has_many :descriptors
end

class ConsolidateDescriptors < ActiveRecord::Migration
  def self.up
    add_column :audio_works, :descriptor_tempo_median, :float
    add_column :audio_works, :descriptor_tempo_average, :float

    median_type = DescriptorType.where(:name => 'tempo median').first
    average_type = DescriptorType.where(:name => 'tempo average').first

    if median_type
      id = median_type.id
      AudioWorks.all.each do |w|
        d = w.descriptors.where(:descriptor_type_id => id).first
        w.update_attribute(:descriptor_tempo_median, d.value) if d
      end
    end

    if average_type
      id = average_type.id
      AudioWorks.all.each do |w|
        d = w.descriptors.where(:descriptor_type_id => id).first
        w.update_attribute(:descriptor_tempo_average, d.value) if d
      end
    end

    drop_table :descriptor_types
    drop_table :descriptors
  end

  def self.down
    create_table :descriptor_types do |t|
      t.column :name, :string
    end

    create_table :descriptors do |t|
      t.column :descriptor_type_id, :integer
      t.column :audio_work_id, :integer
      t.column :value, :float
    end

    remove_column :audio_works, :descriptor_tempo_median
    remove_column :audio_works, :descriptor_tempo_average
  end
end
