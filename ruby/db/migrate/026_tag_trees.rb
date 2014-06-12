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

class Tag < ActiveRecord::Base
end

class TagClass < ActiveRecord::Base
  has_many :tags
end

class TagTrees < ActiveRecord::Migration
  def self.up
    tree = Hash.new
    TagClass.all.each do |c|
      #store the new parent id for the tags
      t = Tag.create(:name => c.name, :tag_class_id => 0)
      tree[t.id] = c.tags.collect
      t.save
    end

    rename_column :tags, :tag_class_id, :parent_id
    Tag.reset_column_information
    tree.each do |id, tags|
      tags.each { |t| t.update_attribute(:parent_id, id) }
    end

    drop_table :tag_classes
  end

  def self.down
  end
end

