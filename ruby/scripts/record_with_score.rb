#!/usr/bin/env ruby

require 'rubygems'
require 'ruby-osc'

SERVER_PORT = 10010
@@start_time = Time.now

jack_ports = ['datajockey:output0', 'datajockey:output1']
audio_file_out = "datajockey_capture-#{@@start_time.to_s.gsub(" ", "_")}.wav"

@@rec_process = fork do
  exec('jack_capture', '-dc', '-c', jack_ports.size.to_s, '-fn', audio_file_out, *jack_ports.collect { |p| ['--port', p] }.flatten)
end

def evt_time
  Time.now - @@start_time
end

def quit_app
  Process.kill("TERM", @@rec_process)
  Process.wait
  exit
end

trap("SIGINT") do
  quit_app
end

puts "##{@@start_time} #{audio_file_out}"

OSC.run do
  server = OSC::Server.new SERVER_PORT

  server.add_pattern %r{/dj/player/.*} do |*args| # this will match any /foo node
    player = args[0].split('/')[3].to_i
    cmd = args[0].sub(%r{/dj/player/\d/}, "").gsub("/", "_")

    case cmd
    when 'audible'
      audible = (args[1] == 1)
      puts "#{evt_time} #{player} audible #{audible}"
    when 'audio_file'
      puts "#{evt_time} #{player} audio_file #{args[1]}"
    when 'load'
      puts "#{evt_time} #{player} load #{args[1]}" if args.size == 2
    end
  end

  server.add_pattern '/dj/quit' do |*args| 
    quit_app
  end
end
