#!/usr/bin/env ruby

require File.expand_path("../vym-ruby", __FILE__) 
require 'tempfile'
require 'mail'

mail_in = Tempfile.new("mail")
begin
  mail_in.write(ARGF.read)
  mail_in.rewind
  out = Tempfile.new("temp")
  begin
    mail = Mail.read(mail_in.path)

    puts "is multipart: #{mail.multipart?}"

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
      puts mail.parts.first.decoded
      out << mail.parts.first.decoded
    else
      out << mail.decoded
    end

    out << "</body></html>"

    out.rewind

    vym_mgr = VymManager.new
    vym = vym_mgr.find('production') 

    if !vym
      puts "Couldn't find instance name \"#{instance_name}\", please start one:"
      puts "vym -l -n \"#{instance-name}\" -t test/default.vym"
      exit
    end

    # Before doing anything, make sure there is a return value available
    # Otherwise the script might block     // FIXME-1
    version = vym.version

    map = vym.currentMapX();
    #map = vym.map(6)
    #map.select "mc:0,b0:0,bo:0"

    map.addBranch()
    map.selectLatestAdded

    date = mail.date.to_s.gsub!(/T.*$/,"")
    map.setHeadingPlainText("Email #{date}: #{mail.subject}")
    map.loadNote(out.path)
    map.colorBranch("#0000ff")
  ensure
    out.close
    out.unlink
  end

ensure
  mail_in.close
  mail_in.unlink
end
