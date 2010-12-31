$LOAD_PATH << File.dirname(__FILE__)
$LOAD_PATH << File.join(File.dirname(__FILE__), '..', 'audio', 'wrapper')
STYLE_SHEET_FILE = 'style.css'

require 'view/mixerpanel'
require 'datajockey_audio'

include DataJockey

app = Qt::Application.new(ARGV)
app.setFont(Qt::Font.new('Times', 10, Qt::Font::Bold))

mixer_panel = View::MixerPanel.new
mixer_panel.show()

audio_controller = Audio::AudioController.instance
#set the parent otherwise queued connections are lost
audio_controller.set_parent(app)

if File.exists?(STYLE_SHEET_FILE)
  app.set_style_sheet(File.read(STYLE_SHEET_FILE))
end

mixer_panel.players.each_with_index do |player, index|
  player.buttons[:pause].on(:toggled) do |t|
    audio_controller.set_player_pause(index, t)
  end
  player.volume_slider.on(:value_changed) do |v|
    audio_controller.set_player_volume(index, v)
  end
  player.buttons[:load].on(:released) do |t|
    filename = Qt::FileDialog.get_open_file_name(mixer_panel, "load audio file", "/home/alex/music/new/martial_canterel/confusing_outsides/", "Audio Files (*.flac *.ogg *.mp3 *.wav)")
    if filename and not filename.empty?
      audio_controller.set_player_audio_file(index, filename)
    end
  end
end

audio_controller.on(:player_audio_file_load_progress, ["int", "int"]) do |player_index, percent|
  if (player = mixer_panel.players[player_index])
    player.progress[:load].show
    player.progress[:load].value = percent
  end
end

audio_controller.on(:player_audio_file_changed, ["int", "QString"]) do |player_index, file|
  if (player = mixer_panel.players[player_index])
    player.progress[:load].hide
  end
end

audio_controller.on(:player_volume_changed) do |player_index, value|
  if (player = mixer_panel.players[player_index])
    player.volume_slider.value = value
  end
end

app.on(:about_to_quit) do
  audio_controller.stop_audio
end

#TODO
audio_controller.start_audio
app.exec()

