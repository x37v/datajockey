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

class DescriptorsFloatValueOnly < ActiveRecord::Migration
  def self.up
    #work around for sqlite issues with table name lengths, remove descriptor indices
    #doesn't seem
    if ActiveRecord::Base.retrieve_connection.adapter_name.downcase =~ /sqlite/
      remove_index(:descriptors, [:audio_work_id, :descriptor_type_id])
      remove_index(:descriptors, [:audio_work_id])
    end

    rename_column :descriptors, :float_value, :value
    remove_column :descriptors, :int_value

    if ActiveRecord::Base.retrieve_connection.adapter_name.downcase =~ /sqlite/
      add_index(:descriptors, [:audio_work_id])
      add_index(:descriptors, [:audio_work_id, :descriptor_type_id])
    end
  end

  def self.down
    #work around for sqlite issues with table name lengths, remove descriptor indices
    if ActiveRecord::Base.retrieve_connection.adapter_name.downcase =~ /sqlite/
      remove_index(:descriptors, [:audio_work_id, :descriptor_type_id])
      remove_index(:descriptors, [:audio_work_id])
    end

    rename_column :descriptors, :value, :float_value
    add_column :descriptors, :int_value, :integer

    if ActiveRecord::Base.retrieve_connection.adapter_name.downcase =~ /sqlite/
      add_index(:descriptors, [:audio_work_id])
      add_index(:descriptors, [:audio_work_id, :descriptor_type_id])
    end
  end
end
