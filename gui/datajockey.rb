$LOAD_PATH << File.dirname(__FILE__)
$LOAD_PATH << File.join(File.dirname(__FILE__), '..', 'audio', 'wrapper')
$LOAD_PATH << File.join(File.dirname(__FILE__), 'ext')
STYLE_SHEET_FILE = 'style.css'

require 'view/mixerpanel'
require 'model/beatbuffer'

include DataJockey

app = Qt::Application.new(ARGV)
app.setFont(Qt::Font.new('Times', 10, Qt::Font::Bold))

mixer_panel = View::MixerPanel.new
mixer_panel.show()

audio_controller = Qt::DBusInterface.new("org.x37v.datajockey", "/audio", 
                                "org.x37v.datajockey.audio",
                                Qt::DBusConnection::sessionBus(), app)

#create the beat buffers
@beat_buffers = mixer_panel.players.size.times.collect { Model::BeatBuffer.new }


=begin
audio_controller = Audio::AudioController.instance
#set the parent otherwise queued connections are lost
audio_controller.set_parent(app)
=end

if File.exists?(STYLE_SHEET_FILE)
  app.set_style_sheet(File.read(STYLE_SHEET_FILE))
end

load_dialog = Qt::FileDialog.new(mixer_panel, 'load audio file').tap { |d|
  d.file_mode = Qt::FileDialog::ExistingFile
  d.name_filter = 'Audio Files (*.flac *.ogg *.mp3 *.wav)'
}

@pausing = Hash.new
mixer_panel.players.each_with_index do |player, index|
  #work with state changes
  player.buttons[:pause].on(:toggled) do |t|
    audio_controller.set_player_pause(index, t)
  end
  player.volume_slider.on(:value_changed) do |v|
    audio_controller.set_player_volume(index, v)
  end
  player.speed_slider.on(:value_changed) do |v|
    audio_controller.set_player_play_speed(index, v + 1000)
  end
  player.buttons[:load].on(:released) do |t|
    if load_dialog.exec == Qt::Dialog::Accepted
      files = load_dialog.selected_files
      audio_controller.set_player_audio_file(index, files.first) if files.size > 0
    end
  end
  player.buttons[:seek_back].on(:pressed) do
    audio_controller.set_player_position_relative(index, -0.4)
  end
  player.buttons[:seek_forward].on(:pressed) do
    audio_controller.set_player_position_relative(index, 0.2)
  end

  #TODO is there a way to make this less hackish?
  @pausing[index] = audio_controller.player_pause(index)
  player.waveform_view.on(:press) do
    @pausing[index] = audio_controller.player_pause(index)
    audio_controller.set_player_pause(index, true)
  end
  player.waveform_view.on(:release) do
    unless @pausing[index] 
      audio_controller.set_player_pause(index, false)
    end
  end
  player.waveform_view.on(:seeking) do |frames|
    audio_controller.set_player_position_frame_relative(index, frames.to_i)
  end

  #grab current state
  player.volume = audio_controller.player_volume(index)
  player.audio_file = audio_controller.player_audio_file(index)
  player.play_speed = audio_controller.player_play_speed(index)
  player.audio_file_position = audio_controller.get_player_position_frame(index)
end

audio_controller.on(:player_audio_file_load_progress, ["int", "int"]) do |player_index, percent|
  if (player = mixer_panel.players[player_index])
    player.progress[:load].show
    player.progress[:load].value = percent
  end
end

audio_controller.on(:player_audio_file_changed, ["int", "QString"]) do |player_index, file|
  if (player = mixer_panel.players[player_index])
    player.audio_file = file
    player.progress[:load].hide
  end
end

audio_controller.on(:player_volume_changed) do |player_index, value|
  if (player = mixer_panel.players[player_index])
    #TODO need to find way to not get into a recursive loop here..
    #player.volume_slider.value = value
  end
end

audio_controller.on(:player_position_changed, ["int", "int"]) do |player_index, pos|
  if (player = mixer_panel.players[player_index])
    player.audio_file_position = pos
  end
end

@beat_buffers.each_with_index do |buffer, index|
  buffer.on(:timepoints_changed) do |timepoints|
    audio_controller.set_player_beat_buffer_begin(index)
    audio_controller.set_player_beat_buffer_clear(index)
    timepoints.each do |t|
      audio_controller.set_player_beat_buffer_add_beat(index, t)
    end
    audio_controller.set_player_beat_buffer_end(index, true)
  end
end

#app.on(:about_to_quit) do
#  audio_controller.stop_audio
#end

#audio_controller.start_audio
=begin
#looks like it has to do with having the audio thread running in the same process as the qt ruby gui stuff
audio_controller.set_player_audio_file(0, '/home/alex/backup/alex_big_harddrive/alex/music-backup/adam_x/sync_jacks_trax/03-octane_propellant.flac')
audio_controller.set_player_audio_file(1, '/home/alex/backup/alex_big_harddrive/alex/music-backup/adam_x/sync_jacks_trax/03-octane_propellant.flac')
sleep(4)

mixer_panel.players.first.set_audio_file('/home/alex/backup/alex_big_harddrive/alex/music-backup/adam_x/sync_jacks_trax/03-octane_propellant.flac')
mixer_panel.players[1].set_audio_file('/home/alex/backup/alex_big_harddrive/alex/music-backup/adam_x/sync_jacks_trax/03-octane_propellant.flac')
pos = 0
x = Qt::Timer.every(10) {
  pos += 441
  mixer_panel.players.first.set_audio_file_position(pos)
  mixer_panel.players[1].set_audio_file_position(pos)
}
=end

#audio_controller.set_player_audio_file(0, '/home/alex/backup/alex_big_harddrive/alex/music-backup/drexciya/drexciya_4-the_unknown_aqua_zone/04-aquabahn.flac')

audio_controller.set_player_audio_file(0, '/home/alex/personal/11-phuture-acid_tracks.flac')
audio_controller.set_player_audio_file(1, '/home/alex/personal/11-phuture-acid_tracks.flac')

audio_controller.set_player_sync(0, true)
audio_controller.set_player_sync(1, true)

audio_controller.set_master_bpm(120.0)

@beat_buffers[0].load_beats('/home/alex/personal/projects/datajockey/acid_tracks.yaml')
@beat_buffers[1].load_beats('/home/alex/personal/projects/datajockey/acid_tracks.yaml')

trap("INT") do
  app.quit
end

app.exec()

