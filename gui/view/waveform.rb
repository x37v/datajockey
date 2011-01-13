require 'datajockey_view'

class DataJockey::View::WaveForm < Qt::GraphicsView
  attr_accessor :follow, :full_view

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
    @orientation = :horizontal
    @follow = true
    @full_view = false

    set_background_brush(Qt::Brush.new(Qt::Color.new(0,0,0)))
  end

  def orientation=(orient)
    reset_matrix
    if (orient == :vertical)
      @orientation = :vertical
      rotate(-90)
    else
      @orientation = :horizontal
    end
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

  def resizeEvent(event)
    s = event.size
    reset_matrix
    if @full_view
      if @orientation == :vertical
        frames = @waveform.audio_file_frames
        if frames > 0
          #TODO why 4?
          @waveform.zoom = frames.to_f / (s.height.to_f  * 4)
          scale(s.width.to_f / 200, 1.0)
          center_on(frames.to_f * @waveform.zoom.to_f / 2, 0)
          rotate(-90)
          self.ensure_visible(@scene.scene_rect)
        end
      else
        fit_in_view(@waveform, Qt::IgnoreAspectRatio)
        scale(1.0, s.height.to_f / 200)
      end
    else
      if @orientation == :vertical
        scale(s.width.to_f / 200, 1.0)
        rotate(-90)
      else
        scale(1.0, s.height.to_f / 200)
      end
    end
    super(event)
  end

  def audio_file=(filename)
    @waveform.set_audio_file(filename)
    rect = @waveform.bounding_rect
    unless @full_view
      if transform.is_rotating
        rect.width += 2 * geometry.height
        rect.x -= geometry.height
      else
        rect.width += 2 * geometry.width
        rect.x -= geometry.width
      end
    end
    @scene.set_scene_rect(rect)
    audio_file_position = 0
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
    if @follow
      center_on(x + offset, 0)
    end
    @cursor.set_pos(x, 0)
  end

end
