#!/usr/bin/env ruby
$: << ".."

require 'yaml'
require './subsonic'
require 'datajockey/base'
require 'datajockey/db'

MUSIC_BASE = "/home/alex/music"

config = YAML.load_file("/home/alex/.datajockey/subsonic.yaml")
s = Subsonic.new(config['server'], config['user'], config['password'])

#connect to the database
Datajockey::connect
include Datajockey

s.playlists.each do |pl|
  name = pl["name"]

  if name =~ /\Amix-/
    class_name = 'mix'
    tag_name = name.sub(/\Amix-/, "")
  elsif name =~ /\Atag-/
    class_name = 'tag'
    tag_name = name.sub(/\Atag-/, "")
    if name =~ /\:/
      class_name, tag_name = tag_name.split(/\:/)
    end
  else
    next
  end


  c = TagClass.find_or_create_by_name(class_name)
  t = Tag.find_or_create_by_name_and_tag_class_id(tag_name, c.id)

  puts "#{class_name}:#{tag_name}"
  s.playlist(pl["id"])["entry"].each do |e|
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

