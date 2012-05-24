=begin

  Copyright (c) 2008 Alex Norman.  All rights reserved.
	http://www.x37v.info/datajockey

	This file is part of Data Jockey.
	
	Data Jockey is free software: you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation, either version 3 of the License, or (at your
	option) any later version.
	
	Data Jockey is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
	Public License for more details.
	
	You should have received a copy of the GNU General Public License along
	with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
=end

#XXX eventually it would be nice to test to see if we're in the graphical application

begin
  require 'datajockey'
  require 'datajockey/base'
rescue LoadError => e
	STDERR.puts e
  STDERR.puts "\n\n*******************************************"
  STDERR.puts "cannot load DataJockey library for Ruby, make sure you have it installed"
  STDERR.puts "ruby intepreter will not execute properly"
  STDERR.puts "*******************************************\n\n"
  loop{ sleep(1) }
end


djclassfiles = [
  "applicationmodel",
  "interpreter",
  "mixerchannelmodel",
  "mixerpannelmodel",
  "workfilter",
]

#before we install the files are in "ruby"
if File.directory?("ruby") and File.exists?("ruby/applicationmodel.rb")
  djclassfiles = djclassfiles.collect {|f| File.join("ruby", f)}
else
  djclassfiles = djclassfiles.collect {|f| File.join("datajockey", f)}
end

#so we can see what it is and print the error
curlib = nil
begin
  curlib = 'irb'
  require 'irb'
  djclassfiles.each do |lib|
    curlib = lib
    require lib
  end
rescue LoadError
  STDERR.puts "\n\n*******************************************"
  STDERR.puts "cannot load ruby library \"#{curlib}\" which datajockey requires"
  STDERR.puts "ruby intepreter will not execute properly"
  STDERR.puts "*******************************************\n\n"
  loop{ sleep(1) }
end

# mercilessly borrowed from: http://errtheblog.com/posts/9-drop-to-irb
#
# I've cleaned up a few things and moved some stuff around, not tested at all of course.
#
# Usage:
#
# raggi@mbk:~/dev/gosu$ ruby -rraggi/irb/drop -e "puts 'hi'; dROP! []; puts 'bye'"hi
# Helper Methods: continue, instance, meths, quit!
# Variables: object # => [], binding # => #<Binding:0x39c9c>
# >> self.concat [:foo,:bar,:baz]
# => [:foo, :bar, :baz]
# >> reverse!
# => [:baz, :bar, :foo]
# >> continue
# bye

class RedirectOutput < IO
    def initialize
        super(2)  
    end
    def write(text)
      Datajockey::InterpreterIOProxy::addToOutput(text.to_s)
    end
end

#redirect stdout
$stdout = RedirectOutput.new

#here we read from datajockey 
class DataJockeyInput < IRB::InputMethod
  def gets()
    until Datajockey::InterpreterIOProxy::newInput
      sleep(0.001)
    end
    input = Datajockey::InterpreterIOProxy::getInput + "\n"
    raise RubyLex::TerminateLineInput if input =~ /DATAJOCKEY_CANCEL_INPUT/
    return input
  end
  def readable_atfer_eof?()
    true
  end
end

module IRB
  class <<self
    attr_accessor :instance
  end

  def self.set_binding(binding)
    old_args = ARGV
  	ARGV.size.times { ARGV.shift }
    
    IRB.setup(nil)

    workspace = WorkSpace.new(binding)

    if @CONF[:SCRIPT]
      self.instance = Irb.new(workspace, @CONF[:SCRIPT])
    else
      self.instance = Irb.new(workspace, DataJockeyInput.new)
      #self.instance = Irb.new(workspace)
    end

    @CONF[:IRB_RC].call(self.instance.context) if @CONF[:IRB_RC]
    @CONF[:MAIN_CONTEXT] = self.instance.context

    trap 'INT' do
      self.instance.signal_handle
    end
  ensure
    old_args.each { |a| ARGV << a }
  end
  
  def self.start_session(binding)
    old_args = ARGV
  	ARGV.size.times { ARGV.shift }

    set_binding(binding)

    catch(:IRB_EXIT) do
      self.instance.eval_input
    end
  ensure
    old_args.each { |a| ARGV << a }
  end
end

module IRBHelper
  def meths(o); puts (o.methods.sort - Class.new.methods).join("\n"); end
  #def quit!; irb_exit; ::Process.exit!; end
  #def instance; IRB.instance; end
  #def continue; irb_exit; end
end

if defined? IRBHelper
  puts "Helper Methods: #{(Class.new.instance_eval {include IRBHelper;self}.new.methods.sort - Class.new.methods).join(', ')}"
  include IRBHelper
end

#process events coming from data jockey
Thread.start {
  loop {
    sleep(0.001)
    #process the incoming events
    Datajockey::InterpreterIOProxy.processEvents
  }
}

if File.exists?(Datajockey::Configuration::getFile)
  Datajockey::setConfFile(Datajockey::Configuration::getFile)
else
  STDERR.puts "\n\n*******************************************"
  STDERR.puts "cannot find datajockey config.yaml file"
  STDERR.puts "you will not be able to interact with the DB in the interpreter"
  STDERR.puts "*******************************************\n\n"
  loop{ sleep(1) }
end

begin
  Datajockey::connect
rescue
  puts $!
end

#load the startup file if there is one
begin
  if File.exists?(File.expand_path("~/.datajockey/startup.rb"))
    load File.expand_path("~/.datajockey/startup.rb")
  end
rescue
  puts $!
end

#include the Datajockey module so we don't have to prefix everything with Datajockey
include Datajockey

#set the binding
IRB.set_binding(Datajockey::ApplicationModel.instance)

#evaluate the input [run the interp]
loop {
  catch(:IRB_EXIT) do
    IRB.instance.eval_input
  end
}

