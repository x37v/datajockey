$LOAD_PATH << File.dirname(__FILE__)
$LOAD_PATH << File.join(File.dirname('__FILE__'), '..')
STYLE_SHEET_FILE = 'style.css'

#require 'view/mixerpanel'
require 'lib/qt'

app = Qt::Application.new(ARGV)

require 'datajockey_view'
w = DataJockey::View::WaveFormView.new {
  set_pen(Qt::Pen.new(Qt::Color.new(255,0,0)))
}

s = Qt::GraphicsScene.new
s.add_item(w)
cursor = Qt::GraphicsLineItem.new(0, -100, 0, 100) {
  set_pen(Qt::Pen.new(Qt::Color.new(0,255,0)))
}
s.add_item(cursor)
v = Qt::GraphicsView.new(s)

timer = Qt::Timer.every(10) {
  cursor.move_by(441, 0)
  v.center_on(cursor)
}


top = Qt::Widget.new
layout = Qt::VBoxLayout.new
layout << v
top.set_layout layout

=begin
v2 = Qt::GraphicsView.new(s)
v2.rotate(90)
layout << v2
=end

top.show

app.exec()

