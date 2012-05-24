=begin

  Copyright (c) 2008 Alex Norman.  All rights reserved.
	http://www.x37v.info/datajockey

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

module Datajockey
  #This class provides a way to filter audio works based on some attribute of
  #those works.
  class WorkFilter
    #redefining the work filter init method.
    #we create a proc that calls our 'works' method
    #and pass this proc to the original initialize method.
    alias :old_init :initialize
    #Takes a 'filter name,' which identifies it in the GUI, and a 'description'
    #of the filter, which gives users a description of what the filter does.
    def initialize(name, description = "Description of WorkFilter")
      this_obj = self
      @proc = Proc.new { 
        clearWorks
        this_obj.works.each do |i|
          if i.is_a?(Integer)
            addWork(i)
          end
        end
      }
      old_init(name, description, @proc)
    end
    #Overload this method in order to get your filters to work.  Return an
    #array of work ids.
    def works
      return []
    end
  end
end

=begin
#this is an example filter that simply returns a random number
class RandFilter < Datajockey::WorkFilter
  def initialize
    super("Random Filter", "This Filter gives one Random Work.")
  end
  def works
    return [rand(400)]
  end
end
=end
