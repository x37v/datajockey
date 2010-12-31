# Copyright (c) 2009 Paolo Capriotti <p.capriotti@gmail.com>
#
# Edited a bit by Alex...
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

require 'Qt4'
require 'lib/observer_utils'

#this is by alex
class Qt::Base
  double_arrow = self.instance_method(:<<)

  #THIS IS A HACK, ruby-qt's inheritance is effed
  define_method(:<<) do |arg|
    #only expose this for layouts
    unless self.is_a?(Qt::Layout)
      double_arrow.bind(self).call(arg)
    else
      unless arg.is_a?(Array)
        arg = [arg]
      end
      arg.each do |i|
        if i.is_a?(Qt::LayoutItem)
          self.add_layout i
        else
          self.add_widget i
        end
      end
      self
    end
  end
end

class Qt::Variant
  # 
  # Convert any marshallable ruby object into a QVariant.
  # 
  def self.from_ruby(x)
    new(Marshal.dump(x))
  end

  # 
  # Extract the ruby object contained in a QVariant.
  # 
  def to_ruby
    str = toString
    Marshal.load(str) if str
  end
end

class Qt::ByteArray
  def self.from_hex(str)
    new([str.gsub(/\W+/, '')].pack('H*'))
  end
end

class Qt::Painter
  # 
  # Ensure this painter is closed after the block is executed.
  # 
  def paint
    yield self
  ensure
    self.end
  end
  
  # 
  # Execute a block, then restore the painter state to what it
  # was before execution.
  # 
  def saving
    save
    yield self
  ensure
    restore
  end
end

class Qt::Image
  # 
  # Convert this image to a pixmap.
  # 
  def to_pix
    Qt::Pixmap.from_image self
  end
  
  # 
  # Paint on an image using the given block. The block is passed
  # a painter to use for drawing.
  # 
  def self.painted(size, &blk)
    Qt::Image.new(size.x, size.y, Qt::Image::Format_ARGB32_Premultiplied).tap do |img|
      img.fill(0)
      Qt::Painter.new(img).paint(&blk)
    end
  end

  # 
  # Render an svg object onto a new image of the specified size. If id is not
  # specified, the whole svg file is rendered.
  # 
  def self.from_renderer(size, renderer, id = nil)
    img = Qt::Image.painted(size) do |p| 
      if id
        renderer.render(p, id)
      else
        renderer.render(p)
      end
    end
    img
  end
end

class Qt::MetaObject
  def create_signal_map
    map = {}
    (0...methodCount).map do |i|
      m = method(i)
      if m.methodType == Qt::MetaMethod::Signal
        sign = m.signature 
        sign =~ /^(.*)\(.*\)$/
        sig = $1.underscore.to_sym
        val = [sign, m.parameterTypes]
        map[sig] ||= []
        map[sig] << val
      end
    end
    map
  end
end

class Qt::Base
  include Observable
  
  class SignalDisconnecter
    def initialize(obj, sig)
      @obj = obj
      @sig = sig
    end
    
    def disconnect!
      @obj.disconnect(@sig)
    end
  end
  
  class ObserverDisconnecter
    def initialize(obj, observer)
      @obj = obj
      @observer = observer
    end
    
    def disconnect!
      @obj.delete_observer(@observer)
    end
  end
  
  class Signal
    attr_reader :symbol

    def initialize(signal, types)
      raise "Only symbols are supported as signals" unless signal.is_a?(Symbol)
      @symbol = signal
      @types = types
    end

    def self.create(signal, types)
      if signal.is_a?(self)
        signal
      else
        new(signal, types)
      end
    end
    
    def to_s
      @symbol.to_s
    end
  end

  def on(sig, types = nil, &blk)
    sig = Signal.create(sig, types)
    candidates = if is_a? Qt::Object
      signal_map[sig.symbol]
    end
    if candidates
      if types
        # find candidate with the correct argument types
        candidates = candidates.find_all{|s| s[1] == types }
      end
      if candidates.size > 1
        # find candidate with the correct arity
        arity = blk.arity
        if blk.arity == -1
          # take first
          candidates = [candidates.first]
        else
          candidates = candidates.find_all{|s| s[1].size == arity }
        end
      end
      if candidates.size > 1
        raise "Ambiguous overload for #{sig} with arity #{arity}"
      elsif candidates.empty?
        msg = if types
          "with types #{types.join(' ')}"
        else
          "with arity #{blk.arity}"
        end
        raise "No overload for #{sig} #{msg}"
      end
      sign = SIGNAL(candidates.first[0])
      connect(sign, &blk)
      SignalDisconnecter.new(self, sign)
    else
      observer = observe(sig.symbol, &blk)
      ObserverDisconnecter.new(self, observer)
    end
  end

  def in(interval, &blk)
    Qt::Timer.in(interval, self, &blk)
  end

  def run_later(&blk)
    self.in(0, &blk)
  end
  
  def signal_map
    self.class.signal_map(self)
  end
  
  def self.signal_map(obj)
    @signal_map ||= self.create_signal_map(obj)
  end
  
  def self.create_signal_map(obj)
    obj.meta_object.create_signal_map
  end
end

class Qt::Timer
  # 
  # Execute the given block every interval milliseconds and return a timer
  # object. Note that if the timer is garbage collected, the block will not
  # be executed anymore, so the caller should keep a reference to it for as
  # long as needed.
  # To prevent further invocations of the block, use QTimer#stop.
  # 
  def self.every(interval, &blk)
    time = Qt::Time.new
    time.restart
    
    timer = new
    timer.connect(SIGNAL('timeout()')) { blk[time.elapsed] }
    timer.start(interval)
    # return the timer, so that the caller
    # has a chance to keep it referenced, so
    # that it is not garbage collected
    timer
  end

  # 
  # Execute the given block after interval milliseconds. If target is
  # specified, the block is invoked in the context of target.
  # 
  def self.in(interval, target = nil, &blk)
    single_shot(interval,
                Qt::BlockInvocation.new(target, blk, 'invoke()'),
                SLOT('invoke()'))
  end
end

