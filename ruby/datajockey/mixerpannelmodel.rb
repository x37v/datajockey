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
  #a class which contains several mixers and a master
  class MixerPanelModel
    #gives an array of mixers which have works audible on the main outs
    def audibleMixers
      mixers = []
      self.numMixers.times { |i|
        if self.mixerAudible(i)
          mixers << i
        end
      }
      return mixers
    end
    #gives an array of ids of works which are audible on the main outs
    def audibleWorks
      self.audibleMixers.collect { |i| self.mixer(i).work }
    end
  end
end
