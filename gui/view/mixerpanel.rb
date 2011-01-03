require 'view/base'
require 'view/player'

class DataJockey::View::MixerPanel < Qt::Widget
  attr_reader :players
  DEFAULT_NUM_PLAYERS = 2

  def initialize(parent = nil)
    super(parent)
    @players = Array.new

    DEFAULT_NUM_PLAYERS.times do |i|
      @players << 
      if i.odd?
        View::Player.new(:waveform_position => :left)
      else
        View::Player.new
      end
    end

    @top_layout = Qt::HBoxLayout.new.tap do |l|
      l << @players
    end

    set_layout @top_layout
  end
end
