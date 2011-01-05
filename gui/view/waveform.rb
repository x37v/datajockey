require 'datajockey_view'
require 'forwardable'

class DataJockey::View::WaveForm < Qt::GraphicsView
  extend Forwardable

  def initialize
    super
    @waveform = DataJockey::View::WaveFormItem.new.tap { |i|
      i.set_pen(Qt::Pen.new(Qt::Color.new(255,0,0)))
      i.set_zoom(150)
    }

    @cursor = Qt::GraphicsLineItem.new(0, -100, 0, 100).tap { |i|
      i.set_pen(Qt::Pen.new(Qt::Color.new(0,255,0)))
    }

    @scene = Qt::GraphicsScene.new { |s|
      s.add_item(@waveform)
      s.add_item(@cursor)
    }

    self.set_scene(@scene)

    set_background_brush(Qt::Brush.new(Qt::Color.new(0,0,0)))
  end

  def audio_file=(filename)
    @waveform.set_audio_file(filename)
  end

  def clear_audio_file
    @waveform.clear_audio_file()
  end

  def audio_file_position=(frame_index)
    x = frame_index / @waveform.zoom
    center_on(x + 100, 0)
    @cursor.set_pos(x, 0)
  end

end
