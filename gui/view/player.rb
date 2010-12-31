require 'view/base'
require 'view/eq'

class DataJockey::View::Player < Qt::Widget
  attr_reader :buttons, :eq, :volume_slider, :progress
  RANGE_HEADROOM = DataJockey::View::INT_MULT / 3
  VOLUME_MAX = RANGE_HEADROOM + DataJockey::View::INT_MULT

  def initialize
    super

    @top_layout = Qt::VBoxLayout.new
    @top_layout.set_alignment Qt::AlignCenter
    set_layout @top_layout

    @button_layout = Qt::HBoxLayout.new
    @button_layout.set_alignment Qt::AlignCenter
    @top_layout << @button_layout

    @buttons = {
      :load => Qt::PushButton.new('load'),
      :pause => Qt::PushButton.new('pause') { |b| 
        b.set_checkable(true) 
      }
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
    @top_layout << @progress[:load]

    @eq = View::Eq.new
    @top_layout << @eq

    @volume_slider = Qt::Slider.new { |s|
      s.set_range(0, VOLUME_MAX)
    }
    @slider_layout = Qt::HBoxLayout.new { |l|
      l.add_stretch
      l.add_widget @volume_slider
      l.add_stretch
    }
    @top_layout << @slider_layout

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
end

