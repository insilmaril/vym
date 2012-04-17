#!/usr/bin/env ruby

require '/suse/uwedr/vym/code/scripts/vym-ruby'
require 'tempfile'

allowed_headers = %w(Subject Date From To)

header = %w()
body = %w()
subject = "";

part="Header"

#FIXME no decoding yet, e.g. from iso 8859-x

$stdin.each do |l| 
  if part=="Header"
    if l =~/^\s$/ 
      part="Body"
    else
      allowed_headers.each {|h|  header << l if l =~/^#{h}:/ }
      subject = $1 if l =~ /^Subject:\s*(.*)$/
    end  
  else  
    body << l
  end  
  #puts "#{part}: #{l}"
end  

note  = header.join
note += body.join

out=Tempfile.new("tempfile")
out << note
out.close

vym_mgr=VymManager.new
vym=Vym.new(vym_mgr.find('production') )

vym.addBranch 
vym.selectLastBranch 
vym.setHeading "#{subject}"
vym.loadNote "#{out.path}"

