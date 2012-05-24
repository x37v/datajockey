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

require 'datajockey_utils'
require 'tempfile'
require 'fileutils'

module Datajockey
  module Annotation
    @@beatroot_jarfile_loc = nil
    def Annotation::getBeatLocations(audioFile)
      unless @@beatroot_jarfile_loc
        beat_root_search = [
          "lib/beatroot-0.5.3.jar",
          "/usr/local/share/datajockey/beatroot-0.5.3.jar",
          "/usr/local/share/datajockey/beatroot.jar",
          "/usr/share/datajockey/beatroot-0.5.3.jar",
          "/usr/share/datajockey/beatroot.jar",
        ]
        #find beatroot jar file!
        beat_root_search.each do |p|
          if File.exists?(p)
            @@beatroot_jarfile_loc = p
            break
          end
        end
        if not @@beatroot_jarfile_loc
          raise "cannot find beatroot jarfile which #{self.name}::getBeatLocations depends on"
        end
      end

      a = Tempfile.new("datajockeybeats")
      tempwav = a.path + ".wav"
      begin
        Datajockey_utils::soundFileToWave(audioFile, tempwav)
        if ENV["DATAJOCKEY_JAVA_OPTS"]
          unless system("java", ENV["DATAJOCKEY_JAVA_OPTS"], "-jar", @@beatroot_jarfile_loc, "-o", a.path, tempwav)
            raise "Cannot execute beatroot, is the jar file in a known location?"
          end
        else
          unless system("java", "-jar", @@beatroot_jarfile_loc, "-o", a.path, tempwav)
            raise "Cannot execute beatroot, is the jar file in a known location?"
          end
        end
      rescue => e
        raise e
      ensure
        if File.exists?(tempwav)
          FileUtils.rm(tempwav)
        end
      end
      return a.readlines.collect{|l| l.to_f}
    end

    def Annotation::smoothNumArray(a, numloops = 1)
      res = Array.new(a)
      numloops.times do
        for i in 1..(res.length - 2) do
          #find the mid point
          mid = (res[i + 1] - res[i - 1]) / 2.0 + res[i - 1]
          #find the difference of the mid point and the actual point
          dif = mid - res[i]
          #add 1/2 of the difference to the actual point
          res[i] += (dif / 2.0)
        end
      end
      return res
    end

  end
end

#puts Datajockey::Annotation::smoothNumArray(Datajockey::Annotation::getBeatLocations("/tmp/test.mp3"), 10).join("\n")
