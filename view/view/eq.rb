require 'view/base'

class DataJockey::View::Eq < Qt::Widget
  attr_reader :dials
  RANGES = [:high, :mid, :low]

  def initialize
    super

    @dials = Hash.new
    RANGES.each do |range|
      @dials[range] = Qt::Dial.new.tap do |d|
        d.set_range(-DataJockey::View::INT_MULT,DataJockey::View::INT_MULT)
        d.set_fixed_size(d.minimum_size_hint)
        d.set_tool_tip("eq #{range}")
      end
    end

    @layout = Qt::VBoxLayout.new.tap do |l|
      l << @dials.values
      l.set_alignment Qt::AlignHCenter
    end
    set_layout @layout
  end

  RANGES.each do |d|
    define_method("#{d}=") do |val|
      dials[d].value = val.to_i
    end
    define_method("#{d}") do
      dials[d].value
    end
  end

end

