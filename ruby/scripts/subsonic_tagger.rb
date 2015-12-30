#!/usr/bin/env ruby
require 'yaml'
require_relative 'subsonic'
require_relative '../datajockey/base'
require_relative '../datajockey/db'

MUSIC_BASE = "/home/alex/music"

config = YAML.load_file("/home/alex/.datajockey/subsonic.yaml")
s = Subsonic.new(config['server'], config['user'], config['password'])

#connect to the database
Datajockey::connect
include Datajockey

s.playlists.each do |pl|
  name = pl["name"]

  mix_prefix = /\A(a*-)?mix-/

  if name =~ mix_prefix
    class_name = 'mix'
    tag_name = name.sub(mix_prefix, "")
  elsif name =~ /\Atag-/
    class_name = 'tag'
    tag_name = name.sub(/\Atag-/, "")
    if name =~ /\:/
      class_name, tag_name = tag_name.split(/\:/)
    end
  else
    next
  end


  c = Tag.find_or_create_by_name_and_parent_id(class_name, 0)
  t = Tag.find_or_create_by_name_and_parent_id(tag_name, c.id)

  puts "#{class_name}:#{tag_name}"
  entries = s.playlist(pl["id"])["entry"]
  unless entries
    puts "\tempty"
    next
  end
  entries.each do |e|
    next unless e.is_a?(Hash)
    f = File.join(MUSIC_BASE, e["path"])
    w = AudioWork.where(:audio_file_location => f).first
    unless w
      puts "\t#{f} not found in db"
      next
    end
    unless w.tags.include?(t)
      w.tags << t
      w.save
    end
  end
end

