require 'view/base'
require 'view/eq'
require 'datajockey_view'

class DataJockey::View::Player < Qt::Widget
  attr_reader :buttons, :eq, :volume_slider, :speed_slider, :progress, :waveform_view
  RANGE_HEADROOM = DataJockey::View::INT_MULT / 3
  VOLUME_MAX = RANGE_HEADROOM + DataJockey::View::INT_MULT

  def initialize(opts = {})
    super()

    opts = {:waveform_position => :right}.merge(opts)

    @control_layout = Qt::VBoxLayout.new
    @control_layout.set_alignment Qt::AlignCenter

    @button_layout = Qt::HBoxLayout.new
    @button_layout.set_alignment Qt::AlignCenter
    @control_layout << @button_layout

    @buttons = {
      :load => Qt::PushButton.new('load'),
      :pause => Qt::PushButton.new('pause') { set_checkable(true) },
      :seek_back => Qt::PushButton.new('<<') { set_auto_repeat(true) },
      :seek_forward => Qt::PushButton.new('>>') { set_auto_repeat(true) }
    }
    @buttons.each do |key, btn|
      btn.set_tool_tip(key.to_s)
    end
    @button_layout << @buttons.values

    @progress = {
      :load => Qt::ProgressBar.new { |p|
        p.alignment = Qt::AlignHCenter
        p.hide
      }
    }
    @control_layout << @progress[:load]

    @eq = View::Eq.new
    @control_layout << @eq

    @speed_slider = Qt::Slider.new(Qt::Horizontal) { |s|
      s.set_range(-300, 300)
      s.set_tick_position(Qt::Slider::TicksBelow)
      s.tick_interval = 50
    }

    @control_layout << @speed_slider

    @volume_slider = Qt::Slider.new { |s|
      s.set_range(0, VOLUME_MAX)
      s.set_tick_position(Qt::Slider::TicksLeft)
      s.tick_interval = 100
    }
    @slider_layout = Qt::HBoxLayout.new { |l|
      l.add_stretch
      l.add_widget @volume_slider
      l.add_stretch
    }
    @control_layout << @slider_layout

    @waveform_item = DataJockey::View::WaveFormItem.new.tap { |i|
      i.set_pen(Qt::Pen.new(Qt::Color.new(255,0,0)))
      i.set_zoom(150)
    }
    @waveform_cursor = Qt::GraphicsLineItem.new(0, -100, 0, 100).tap { |i|
      i.set_pen(Qt::Pen.new(Qt::Color.new(0,255,0)))
    }

    @waveform_scene = Qt::GraphicsScene.new { |s|
      s.add_item(@waveform_item)
      s.add_item(@waveform_cursor)
    }

    @waveform_view = Qt::GraphicsView.new(@waveform_scene).tap { |v|
      v.set_horizontal_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      v.set_vertical_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      v.rotate(-90)
      v.set_background_brush(Qt::Brush.new(Qt::Color.new(0,0,0)))
    }

    @top_layout = Qt::HBoxLayout.new
    @top_layout << @control_layout
    if (opts[:waveform_position] == :right)
      @top_layout << @waveform_view
    else
      @top_layout.insert_widget(0, @waveform_view)
    end
    @waveform_view.hide

    set_layout @top_layout

    @buttons[:pause].on(:toggled) do |t|
      @buttons[:pause].text = t ? 'play' : 'pause'
    end
  end

  def volume=(val)
    @volume_slider.value = val
  end

  def volume
    @volume_slider.value
  end

  def play_speed=(val)
    @speed_slider.value = val - 1000
  end

  def play_speed
    @speed_slider.value + 1000
  end

  def audio_file=(filename)
    @waveform_view.show
    @waveform_item.set_audio_file(filename)
  end

  def clear_audio_file
    @waveform_view.clear_audio_file()
  end

  def audio_file_position=(frame_index)
    x = frame_index / @waveform_item.zoom
    if r = @waveform_scene.scene_rect
      @waveform_view.center_on(x + 100, 0)
    else
      @waveform_view.center_on(x, 0)
    end
    @waveform_cursor.set_pos(x, 0)
  end
end

