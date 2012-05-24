#!/usr/bin/env ruby

#
#how to run: ruby -Iruby/ ./ruby/annotation/create_annotation.rb /tmp/test.mp3
#

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

require 'datajockey/base'
require 'datajockey/annotation/beatlocations'
require 'rubygems'
require 'taglib'
require 'yaml'
require 'datajockey_utils'

class TagLib::File
  def has_attribute(name)
    if !self.send(name) or self.send(name) =~ /^\s*$/
      return false
    else
      return true
    end
  end
  def store_attribute_in_hash(hash, attr_name, newname = nil)
    name = attr_name
    if newname
      name = newname
    end
    if has_attribute(attr_name)
      hash[name] = self.send(attr_name)
    else
      return false
    end
  end
end

class Array
  def median
    a = self.sort
    #if the lenght is odd take the middle one
    mid = a.length / 2
    if a.size % 2 == 1
      return a[mid]
    else
      return (a[mid - 1] + a[mid]).to_f / 2
    end
  end

  def mean
    sum = 0
    self.each do |i|
      sum = sum + i
    end
    return sum.to_f / self.length
  end
end

module Datajockey
  module Annotation
    def Annotation::createAnnotation(audioFile, computeBeats = true, computeDescriptors = true)
      annotation = Hash.new
      if computeBeats or computeDescriptors
        annotation["descriptors"] = Hash.new
      end
      if computeBeats
        smoothing = 1
        if(Datajockey::config["annotation"]["beat locations"]["smoothing"])
          smoothing = Datajockey::config["annotation"]["beat locations"]["smoothing"]
        end

        annotation["beat locations"] = Array.new

        cur = Hash.new
        cur["time points"] = beats = getBeatLocations(audioFile)
        cur["mtime"] = cur["ctime"] = Time.now
        cur["source"] = "Datajockey::Annotation::getBeatLocations(#{audioFile})"
        annotation["beat locations"] << cur

        cur = Hash.new
        cur["time points"] = beats = smoothNumArray(beats, smoothing)
        cur["mtime"] = cur["ctime"] = Time.now
        cur["source"] = 
      "Datajockey::Annotation::smoothNumArray(Datajockey::Annotation::getBeatLocations(#{audioFile}), #{smoothing})"
      annotation["beat locations"] << cur

        if beats.length > 4
          distances = Array.new
          (1..beats.length - 1).each do |i|
            distances << (beats[i].to_f - beats[i - 1].to_f)
          end
          descriptor = Hash.new
          descriptor["median"] = 60.0 / distances.median
          descriptor["average"] = 60.0 / distances.mean
          annotation["descriptors"]["tempo"] = descriptor
        end
      end

      if computeDescriptors
        #get annotation data, we get 3 lines
        #name\twindowSize\thopsize
        #value0\tvalue1\tvalue2..
        #average\tmedian
        descriptor_string = Datajockey_utils::computeDescriptors(audioFile).split("\n");
        (descriptor_string.size / 3).times do |i|
          name, windowSize, hopSize = descriptor_string[i * 3].split("\t")
          values = descriptor_string[i * 3 + 1].split("\t").map{|f| f.to_f}
          average, median = descriptor_string[i * 3 + 2].split("\t").map{|f| f.to_f}

          descriptor = Hash.new
          descriptor["median"] = median
          descriptor["average"] = average
          descriptor["chunks"] = Hash.new
          descriptor["chunks"]["hop size"] = hopSize.to_i
          descriptor["chunks"]["window size"] = windowSize.to_i
          descriptor["chunks"]["values"] = values
          annotation["descriptors"][name] = descriptor
        end
      end

      #get id3tag data from file if it exists
      begin
        tag = TagLib::File.new(audioFile)
        tag.store_attribute_in_hash(annotation,"title")
        tag.store_attribute_in_hash(annotation,"artist")
        tag.store_attribute_in_hash(annotation,"year")
        annotation["channels"] = tag.channels.to_i if tag.has_attribute("channels")
        annotation["seconds"] = tag.length.to_i if tag.has_attribute("length")

        #genre is a tag
        annotation["tags"] = Hash.new
        tag.store_attribute_in_hash(annotation["tags"],"genre")

        #album
        if tag.has_attribute("album")
          annotation["album"] = album = Hash.new
          album["name"] = tag.album
          album["track"] = tag.track.to_i if tag.has_attribute("track")
        end
      rescue TagLib::BadFile
      end

      return annotation
    end
  end
end

if __FILE__ == $0
  Datajockey::setConfFile(File.join(ENV["HOME"], ".datajockey", "config.yaml"))
  puts Datajockey::Annotation::createAnnotation(ARGV[0]).to_yaml
end

#puts Datajockey::Annotation::createAnnotation("/tmp/test.mp3").to_yaml
