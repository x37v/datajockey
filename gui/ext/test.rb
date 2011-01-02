$LOAD_PATH << File.dirname(__FILE__)
$LOAD_PATH << File.join(File.dirname('__FILE__'), '..')
$LOAD_PATH << File.join(File.dirname('__FILE__'), '..', '..', 'audio', 'wrapper')
STYLE_SHEET_FILE = 'style.css'

#require 'view/mixerpanel'
require 'lib/qt'

app = Qt::Application.new(ARGV)

require 'datajockey_audio'
require 'datajockey_view'

audio_controller = DataJockey::Audio::AudioController.instance
audio_controller.set_player_audio_file(0, '/home/alex/music/new/martial_canterel/confusing_outsides/02-confusing_outsides.flac')

audio_controller.set_parent(app)

w = DataJockey::View::WaveFormView.new {
  set_pen(Qt::Pen.new(Qt::Color.new(255,0,0)))
  set_zoom(80)
}

s = Qt::GraphicsScene.new
s.add_item(w)
cursor = Qt::GraphicsLineItem.new(0, -100, 0, 100) {
  set_pen(Qt::Pen.new(Qt::Color.new(0,255,0)))
}
s.add_item(cursor)
v = Qt::GraphicsView.new(s)

#timer = Qt::Timer.every(10) {
#  v.center_on(cursor)
#}


top = Qt::Widget.new
layout = Qt::VBoxLayout.new
layout << v
top.set_layout layout

audio_controller.on(:player_audio_file_changed, ["int", "QString"]) do |player_index, file|
  w.set_audio_file(file)
end

audio_controller.on(:player_position_changed, ["int", "int"]) do |player_index, pos|
  if player_index == 0
    x = pos / w.zoom
    v.center_on(x, 0)
    cursor.set_pos(x, 0)
  end
end

=begin
t = Qt::Timer.in(500) do 
  sleep(1)
  a.set_player_audio_file(0, '/home/alex/music/new/martial_canterel/confusing_outsides/03-fallen_lords.flac')
  sleep(4)
  w.set_audio_file('/home/alex/music/new/martial_canterel/confusing_outsides/03-fallen_lords.flac')
  a.set_player_clear_buffers(0)
  w.set_audio_file(" ")
  sleep(2)
end
=end

=begin
v2 = Qt::GraphicsView.new(s)
v2.rotate(90)
layout << v2
=end

top.show
audio_controller.start_audio

app.on(:about_to_quit) do
  audio_controller.stop_audio
end

app.exec()

