require 'dbus'
require 'pp'

$debug = false

class Vym
  def initialize (name)
    @dbus = DBus::SessionBus.instance
    @service = @dbus.service(name)
    @service.introspect
    @main = @service.object('/vym')
    @main.introspect
    @main.default_iface = "org.insilmaril.vym.main.adaptor"

    # Use metaprogramming to create methods for commands in vym
    # Getting commands for mainwindow via DBUS
    puts "Vym::initialize Retrieving commands via dbus..." if $debug
    s = @main.listCommands
    @vym_commands = s[0].split ","
    @vym_commands.each do |c|
      puts "Creating vym command: #{c}" if $debug
      self.class.send(:define_method, c) do |*pars|
        if pars.length == 0
          # No parameters
          com = "vym.#{c}();"
          puts " * Calling vym: \"#{com}\":" if $debug
          ret = @main.execute( com )
        else  
          # with parameters
          p = "";
          a = []
          pars.each do |p|
            if p.kind_of? String
              a << "'#{p}'"
            else
              a << p
            end
          end  
          com = "vym.#{c} (#{a.join(',')});"
          puts " ** Calling vym: \"#{com}\":" if $debug
          ret = @main.execute( com )
        end  

        #FIXME-0  err = m.errorLevel[0]
        if $debug
          puts "     Returned: #{ret[0]}" if ret[0] != ""
          # puts "        Error: #{err}" if err > 0
        end  
        ret[0]
      end
    end # Creating vym commands
  end

  def mapCount
    @main.mapCount[0]
  end

  def currentMapID
    return @main.currentMapID[0]
  end

  def map (n)
    #puts "def map:  @service.object(\"/vymmodel_#{n}\")"
    map = @service.object("/vymmodel_#{n}")
    map.introspect
    map.default_iface = "org.insilmaril.vym.model.adaptor"

    if mapCount() > 0 && n >= 0
      return VymMap.new(map, n )
    else
      raise "Error: Map #{n} not accessible in #{@instance}!"
    end  
  end

  def show_methods
    puts "Main methods:"
    @main[@main.default_iface].methods.each do |k,v|
      puts " - #{k}"
    end
  end
end


class VymMap
  def initialize(map, n )
    @map = map
    
    # Getting commands for model via DBUS
    #if mapCount() > 0
      # m = model(1)
      s = @map.listCommands
      puts "VymMap::initialize Retrieving commands via dbus..." if $debug
      @model_commands = s[0].split ","
      @model_commands.each do |c|
      #puts "Creating map command: #{c}" if $debug
        self.class.send(:define_method, c) do |*pars|
          if pars.length == 0
            # No parameters
            com = "vym.currentMap().#{c}();"
            puts " * Calling model: \"#{com}\":" if $debug
            ret = @map.execute( com )
          else  
            # Build string with parameters
            p = "";
            a = []
            pars.each do |p|
              if p.kind_of? String
                a << "'#{p}'"
              else
                a << p
              end
            end  
            # com = "vym.clearConsole(); print( vym.currentMap().#{c} (#{a.join(',')}));"
            com = " vym.currentMap().#{c} (#{a.join(',')});"
            puts " ** Calling model: \"#{com}\":" if $debug
            ret = @map.execute( com )
            puts "Done calling" if $debug
          end  

          #FIXME-0 err = m.errorLevel[0]
          if $debug
            puts "     Returned: #{ret[0]}" if ret[0] != ""
            #puts "        Error: #{err}" if err > 0
          end  
          ret[0]
        end
      end
  end # Initialize
end # VymMap

class VymManager
  def initialize
    @dbus = DBus::SessionBus.instance
  end

  def running
    list = @dbus.proxy.ListNames[0].find_all{|item| item =~/org\.insilmaril\.vym/ }
  end

  def show_running
    puts "Running vym instances:\n  #{running.join "\n  "}"
  end

  def find (name)
    list = running
    if list.length == 0
      return nil
    end

    for i in (0...list.length)
      vym_service = @dbus.service(list.at(i))

      vym_main_obj = vym_service.object("/vym");

      vym_main_obj.introspect

      vym_main_obj.default_iface = "org.insilmaril.vym.main.adaptor"

      if vym_main_obj.getInstanceName[0] == name 
        puts "VymManager: Found instance named '#{name}': #{list.at(i)}" if $debug
        return Vym.new list.at(i)
     end  
    end

    raise "Could not find instance named \"test\""
    return nil
  end
end

