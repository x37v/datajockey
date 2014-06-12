require 'mp3info'
require 'shellwords'

def getsamplerate(filepath)
  sr = nil
  if filepath =~ /flac/
    cmd = "sndfile-info"
  elsif filepath =~ /mp3/
    Mp3Info.open(filepath) do |info|
      sr = info.samplerate
    end
  else
    cmd = "exiftool"
  end
  unless sr
    sr = `#{cmd} #{Shellwords.escape(filepath)} | grep "Sample Rate"`.chomp.split(" ").last.to_f
  end
  return sr
end
