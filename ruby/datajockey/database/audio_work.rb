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
  module AudioWorkTagsAssociationExtension
    def [](arg)
      if arg.is_a?(Integer) or arg.is_a?(Range)
        return find(:all, :order => :id)[arg]
      end
      tag_class = TagClass.find_by_name(arg)
      if tag_class
        return find(:all, :conditions => {:tag_class_id => tag_class.id})
      else
        return []
      end
    end
  end
  module AudioWorkDescriptorsAssociationExtension
    #here we make it so that you can do descriptors["name"] or descriptors[index]
    def [](arg)
      if arg.is_a?(Integer) or arg.is_a?(Range)
        #we have an order so that [0] is always [0]
        return find(:all, :order => :id)[arg]
      else
        return find(:first, :conditions => {:descriptor_type_id => DescriptorType.find_by_name(arg)})
      end
    end
  end

  class AudioWork < ActiveRecord::Base
    #has_many :artist_audio_works, :dependent => :destroy
    #has_many :artists, :through => :artist_audio_works

    belongs_to :artist

    has_one :album_audio_work, :dependent => :destroy

    has_many :audio_work_tags, :dependent => :destroy

    has_many :descriptor_types, :through => :descriptors
    has_one :annotation_file, :dependent => :destroy

    has_many :tags, :through => :audio_work_tags, :extend => AudioWorkTagsAssociationExtension
=begin
    #XXX this below doesn't work when we include AudioWork into the Datajockey module, so I
    #explicitly created the association module above and use :extend
    has_many :tags, :through => :audio_work_tags do
      #here we make it so you can do tags["tag name"] or tags[index]
      def [](arg)
        if arg.is_a?(Integer) or arg.is_a?(Range)
          return find(:all, :order => :id)[arg]
        end
        tag_class = TagClass.find_by_name(arg)
        if tag_class
          return find(:all, :conditions => {:tag_class_id => tag_class.id})
        else
          return []
        end
      end
    end
=end

    has_many :descriptors, :dependent => :destroy, :extend => AudioWorkDescriptorsAssociationExtension
=begin
    #XXX this below doesn't work so I created the
    #AudioWorkDescriptorsAssociationExtension above and use :extend
    has_many :descriptors, :dependent => :destroy do
      #here we make it so that you can do descriptors["name"] or descriptors[index]
      def [](arg)
        if arg.is_a?(Integer) or arg.is_a?(Range)
          #we have an order so that [0] is always [0]
          return find(:all, :order => :id)[arg]
        else
          return find(:first, :conditions => {:descriptor_type_id => DescriptorType.find_by_name(arg)})
        end
      end
    end
=end

    #Find the album that an audio work is a part of
    def album
      self.album_audio_work.album
    end

    #Find a work through a regular expression, passed as a hash.
    #default {:artist_name => ".*", :album_name => ".*", :name => ".*"}
    #Example: AudioWork::regex_search({:artist_name => "SPK"})
    def AudioWork.regex_search(params)
      #XXX look for Regexp type!
      if params.is_a?(String)
        params = {:name => params}
      end
      #make sure we have valid parameters
      if !params or !params.is_a?(Hash) or params.size == 0
        return []
      end
      #set up default arguments
      params = {:artist_name => '.*',
        :album_name => '.*',
        :name => '.*'}.merge(params)
      params.each do |key, val|
        if val == ''
          params[key] = '.*'
        end
      end

      works = []
      aw = AudioWork.find(:all, :include => [:artists, :album_audio_work],
                          :conditions => ['audio_works.name REGEXP ? AND artists.name REGEXP ?',
                            params[:name], params[:artist_name]])

      #if you allow any album you should also allow songs with no album
      if params[:album_name] == '.*' or params[:album_name] =~ /^\s*$/
        works = aw
      else
        aw.each do |work|
          if work.album and work.album.name =~ /#{params[:album_name]}/i
            works << work
          end
        end
      end

      return works


      #return AudioWork.find(:all, :include => :artists, 
      #  :conditions => ['audio_works.name REGEXP ? AND artists.name REGEXP ?',
      #    params[:name], params[:artist_name]])

      #return AudioWork.find(:all, :include => [:album_audio_work, :artists], 
      #  :conditions => ['audio_works.name REGEXP ? AND artists.name REGEXP ? AND albums.name REGEXP ?',
      #    params[:name], params[:artist_name], params[:album_name]])
    end
  end
end
