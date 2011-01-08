require 'datajockey_view'

class DataJockey::View::WaveForm < Qt::GraphicsView

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

  def mousePressEvent(event)
    @pos = event.pos
    fire :press
  end

  def mouseMoveEvent(event)
    return unless @pos
    cur_pos = event.pos

    diff = 0
    #we assume it is rotating -90 degrees
    if transform.is_rotating
      diff = cur_pos.y - @pos.y
    else
      diff = @pos.x - cur_pos.x
    end

    frames = @waveform.zoom * diff
    @pos = cur_pos
    fire :seeking => frames
  end

  def mouseReleaseEvent(event)
    @pos = nil
    fire :release
  end

  def wheelEvent(event)
    amt = event.delta / 160.0
    if transform.is_rotating
      amt *= geometry.height
    else
      amt *= geometry.width
    end
    frames = amt * @waveform.zoom
    fire :seeking => frames
  end

  def audio_file=(filename)
    @waveform.set_audio_file(filename)
  end

  def clear_audio_file
    @waveform.clear_audio_file()
  end

  def audio_file_position=(frame_index)
    x = frame_index / @waveform.zoom
    offset = 0
    if transform.is_rotating
      offset = geometry.height / 4
    else
      offset = geometry.width / 4
    end
    center_on(x + offset, 0)
    @cursor.set_pos(x, 0)
  end

end
