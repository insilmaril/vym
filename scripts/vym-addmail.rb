#!/usr/bin/ruby

require File.expand_path("../vym-ruby", __FILE__) 
require 'tempfile'
require 'mail'

mail_in = ""
ARGF.each_line do |line|
  mail_in << line 
end

begin
  out = Tempfile.new("temp")
  begin
    mail = Mail.read_from_string(Mail::Utilities.binary_unsafe_to_crlf(mail_in))
    puts "Mail is multipart: #{mail.multipart?}"

    # Write header
    out << "<html><body>"
    out << "<pre>"
    out << "Subject: #{mail.subject}\n"
    out << "From: #{mail.header[:From]}\n"
    out << "To: #{mail.header[:To]}\n"
    out << "Cc: #{mail.header[:Cc]}\n"
    out << "Date: #{mail.date.to_s}\n"
    out << "</pre>"

    if mail.multipart? then
      puts "Warning: multipart mail detected. Only using first part."
      # puts mail.parts.first.decoded
      out << mail.parts.first.decoded
    else
      #out <<  mail.body.raw_source.gsub("\n", "<br/>")
      out <<  mail.decoded
      #puts out
    end

    out << "</body></html>"

    out.rewind

    vym_mgr = VymManager.new
    vym_mgr.show_running
    vym = vym_mgr.find('production') 

    if !vym
      puts "Couldn't find instance named \"production\", please start one:"
      puts "vym -l -n production -t test/default.vym"

      puts "Currently running vym instances: #{vym_mgr.show_running}"
      exit
    end

    # Before doing anything, make sure there is a return value available
    # Otherwise the script might block     // FIXME-1
    #version = vym.version

#    map = vym.currentMap()
    map = vym.map(1)
    
    #map = vym.map(6)
    #map.select "mc:0,b0:0,bo:0"

    map.addBranch()
    map.selectLatestAdded

    date = mail.date.to_s.gsub!(/T.*$/,"")
    map.setHeadingPlainText("#{date} Email: #{mail.subject}")
    map.loadNote(out.path)
    map.colorBranch("#0000ff")
  ensure
    out.close
    out.unlink
  end

end
