#!/usr/bin/env ruby

require '/suse/uwedr/vym/code/scripts/vym-ruby'
require 'tempfile'

allowed_headers = %w(Subject Date From To)

header = %w()
body = %w()
subject = "";

part="Header"

$stdin.each do |line| 
  if part == "Header"
    if line =~/^\s$/ 
      part="Body"
    else
      allowed_headers.each {|h|  header << line if line =~/^#{h}:/ }
      subject = $1 if line =~ /^Subject:\s*(.*)$/
    end  
  else  
    body << line
  end  
  #puts "#{part}: #{line}"
end  

note  = header.join
note += body.join

out=Tempfile.new("tempfile")
out << note
out.close

vym_mgr = VymManager.new
vym = Vym.new(vym_mgr.find('production') )

vym.addBranch 
vym.selectLastBranch 
vym.setHeading "#{subject}"
vym.loadNote "#{out.path}"
