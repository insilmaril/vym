#!/usr/bin/env ruby

require 'dbus'

$deb = false

class Vym
  def initialize (name)
    @dbus = DBus::SessionBus.instance
    @service = @dbus.service(name)
    @service.introspect
    @main = @service.object('vym')
    @main.introspect
    @main.default_iface = "org.insilmaril.vym.main.adaptor"

    # Use metaprogramming to create methods for commands in vym
    if modelCount > 0
      m= model(1)
      m.default_iface = "org.insilmaril.vym.model.adaptor"
      s=m.listCommands
      @model_commands=s[0].split ","
      @model_commands.each do |c|
	self.class.send(:define_method, c) do |*pars|
	  if pars.length == 0
	    puts " * Calling \"#{c}\":" if $deb
	    ret=m.execute("#{c} ()")
	  else  
	    # Build string with parameters
	    p="";
	    a=[]
	    pars.each do |p|
	      if p.kind_of? String
	        a<<"\"#{p}\""
	      else
	        a<<p
	      end
	    end  
	    puts " * Calling \"#{c} (#{a.join(',')})\":" if $deb
            ret = m.execute("#{c} (#{a.join(',')})")
	  end  

	  err = m.errorLevel[0]
	  if $deb
	    puts "     Returned: #{ret[0]}" if ret[0]!=""
	    puts "        Error: #{err}" if err>0
	  end  
	  ret[0]
	end
      end
    end
  end

  def modelCount
    @main.modelCount[0]
  end

  def model (n)
    if modelCount > 0 && n>=0
      @model = @service.object "vymmodel_#{n}"
      @model.default_iface = "org.insilmaril.vym.model.adaptor"
      return @model
    else
      raise "Error: Model #{n} not accessible in #{@instance}!"
    end  
  end

  def show_methods
    puts "Main methods:"
    @main[@main.default_iface].methods.each do |k,v|
      puts "  #{k}"
    end
    if modelCount > 0
      @model= @service.object 'vymmodel_1'
      @model.default_iface = "org.insilmaril.vym.model.adaptor"
      puts "Model methods:"
      @model[@model.default_iface].methods.each do |k,v|
        puts "  #{k}"
      end
    else
      puts "No model!"
    end  
  end
end

class VymManager
  def initialize
    @dbus = DBus::SessionBus.instance
  end

  def running
    list=@dbus.proxy.ListNames[0].find_all{|item| item =~/org\.insilmaril\.vym/ }
  end

  def show_running
    puts "Running vym instances:\n  #{running.join "\n  "}"
  end

  def find (name)
    list=running
    raise "Could not find running vym instance" if list.length==0

    for i in (0...list.length)
      vym_service=@dbus.service(list.at(i))
      vym_service.introspect
      vym_main_obj = vym_service.object("vym");
      vym_main_obj.introspect

      vym_main_obj.default_iface = "org.insilmaril.vym.main.adaptor"

      if vym_main_obj.getInstanceName[0]==name 
        puts "Found instance named '#{name}': #{list.at(i)}"
	return list.at(i)
      end  
    end
    raise "Could not find instance named \"test\""
  end
end

def expect (comment, v_exp, v_real)
  if v_exp == v_real
    puts "    Ok: #{comment}"
  else  
    puts "Failed: #{comment}. Expected #{v_exp}, but got #{v_real}"
  end  
end

vym_mgr=VymManager.new
#vym_mgr.show_running

vym=Vym.new(vym_mgr.find('test') )
#vym.show_methods
#puts "Modelcount: #{vym.modelCount}"

#vym.moveRel(50,50)
#vym.setHeading("foO","bar")
#puts "Returned: ",vym.getHeading

branch_a="mc:0,bo:0"
branch_b=branch_a+",bo:0"
puts "Select first branch of first mainbranch #{branch_b}"
vym.select branch_b
n=vym.branchCount

puts "Adding branches:"
vym.addBranch()
expect( "Branch count increased for adding a child", n+1, vym.branchCount )

vym.select branch_a
n=vym.branchCount
vym.select branch_b
vym.addBranch(-1)
vym.addBranch(1)
vym.select branch_a
expect( "Branch count increased for adding above and below", n+2, vym.branchCount )

