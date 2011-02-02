require 'yaml'
require 'lib/observer_utils'

module DataJockey
  module Model
    class BeatBuffer
      include Observable

      attr_reader :beat_locations, :file

      def initialize(location = nil)
        @beat_locations = Array.new

        load_beats(location) if location
      end

      def load_beats(location)
        raise "File #{location} not found" unless File.exists?(location)
        y = YAML::load(File.read(location))

        beats = y['beat locations']
        raise "cannot beat locations descriptor in file #{location}" unless beats

        if beats.size > 0
          beats = beats.sort { |a,b| a[:mtime] <=> b[:mtime] }.last
        else
          beats = beats.first
        end

        @file = location

        @beat_locations.clear
        beats['time points'].each do |b|
          @beat_locations << b
        end

        fire :file_changed => @file
        fire :timepoints_changed => @beat_locations

        self
      end

      def clear
        @beat_locations.clear
        fire :cleared
        self
      end

      def update_timepoint(index, value)
        return unless index >= 0 and index < @beat_locations.size and (value.is_a?(Float) or value.is_a?(Integer))
        value = value.tof
        #TODO insure order of beats?
        @beat_locations[index] = value
        fire :timepoint_updated => [index, value]
      end

      def add_timepoint(value)
        return unless value.is_a?(Float) or value.is_a?(Integer)
        value = value.to_f

        #assuming we are inserting at the end
        index = @beat_locations.size
        #insert in order
        if @beat_locations.size == 0 or @beat_locations.last < value
          @beat_locations << value
        else
          @beat_locations.each_with_index do |v,i|
            if v > value
              @beat_locations.insert(i, value)
              index = i
              break
            end
          end
        end

        fire :timepoint_added => [index, value]
      end
    end
  end
end
