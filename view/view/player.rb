require 'view/base'
require 'view/eq'
require 'view/waveform'
require 'forwardable'

class DataJockey::View::Player < Qt::Widget
  extend Forwardable
  attr_reader :buttons, :eq, :volume_slider, :speed_slider, :progress, :waveform_view
  RANGE_HEADROOM = DataJockey::View::INT_MULT / 3
  VOLUME_MAX = RANGE_HEADROOM + DataJockey::View::INT_MULT

  def_delegators :@waveform_view, :audio_file=, :clear_audio_file, :audio_file_position=
  def_delegator :@volume_slider, :value=, :volume=
  def_delegator :@volume_slider, :value, :volume

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

    @waveform_progress = View::WaveForm.new.tap do |w|
      w.set_horizontal_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      w.set_vertical_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      w.orientation = :vertical
      w.follow = false
      w.full_view = true
    end

    @waveform_view = View::WaveForm.new.tap do |w|
      w.set_horizontal_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      w.set_vertical_scroll_bar_policy(Qt::ScrollBarAlwaysOff)
      w.orientation = :vertical
    end

    @top_layout = Qt::HBoxLayout.new
    @splitter = Qt::Splitter.new(Qt::Horizontal)
    @control_widget = Qt::Widget.new
    @control_widget.set_layout @control_layout
    if (opts[:waveform_position] == :right)
      @splitter.add_widget(@control_widget)
      @splitter.add_widget(@waveform_progress)
      @splitter.add_widget(@waveform_view)
    else
      @splitter.add_widget(@waveform_view)
      @splitter.add_widget(@waveform_progress)
      @splitter.add_widget(@control_widget)
    end
    @top_layout << @splitter

    set_layout @top_layout

    @buttons[:pause].on(:toggled) do |t|
      @buttons[:pause].text = t ? 'play' : 'pause'
    end
  end

  def play_speed=(val)
    @speed_slider.value = val - 1000
  end

  def play_speed
    @speed_slider.value + 1000
  end


  def audio_file=(file)
    @waveform_view.audio_file = file
    @waveform_progress.audio_file = file
  end

  def clear_audio_file
    @waveform_view.clear_audio_file
    @waveform_progress.clear_audio_file
  end

  def audio_file_position=(pos)
    @waveform_view.audio_file_position = pos
    @waveform_progress.audio_file_position = pos
  end

end

