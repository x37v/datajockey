#!/usr/bin/env ruby

require 'rubygems'
require 'ruby-osc'

include OSC

jack_ports = ['datajockey:output0', 'datajockey:output1']
audio_file_out = "datajockey_capture-#{Time.now.to_s.gsub(" ", "_")}.wav"

rec_process = fork do
  system('jack_capture', '-dc', '-c', jack_ports.size.to_s, '-fn', audio_file_out, *jack_ports.collect { |p| ['--port', p] }.flatten)
  Kernel.exit!
end

trap("SIGINT") {
  puts "sending hup"
  Process.kill("HUP", rec_process)
  Process.wait
  exit
}

@@start_time = Time.now

def evt_time
  Time.now - @@start_time
end

OSC.run do
  server = Server.new 10010
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
end
