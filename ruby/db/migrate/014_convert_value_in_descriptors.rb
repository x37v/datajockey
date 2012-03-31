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

class ConvertValueInDescriptors < ActiveRecord::Migration
  def self.up
    rename_column(:descriptors, :value, :float_value)
    add_column(:descriptors, :int_value, :integer)
  end

  def self.down
    rename_column(:descriptors, :float_value, :value)
    Descriptor.find(:all).each do |d|
      if d.int_value
        d.update_attribute(:float_value, d.int_value)
      end
    end
    remove_column(:descriptors, :int_value)
  end
end
