#!/usr/bin/ruby

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

    message = mail
    plain_part = mail.multipart? ? (message.text_part ? message.text_part.body.decoded : nil) : message.body.decoded
    html_part = message.html_part ? message.html_part.body.decoded : nil

    if html_part.nil?
      out << "Subject: #{mail.subject}\n"
      out << "From: #{mail.header[:From]}\n"
      out << "To: #{mail.header[:To]}\n"
      out << "Cc: #{mail.header[:Cc]}\n"
      out << "Date: #{mail.date.to_s}\n"
      out << "\n"
      out << plain_part
    else
      out << "<html><body>"
      out << "<pre>"
      out << "Subject: #{mail.subject}\n"
      out << "From: #{mail.header[:From]}\n"
      out << "To: #{mail.header[:To]}\n"
      out << "Cc: #{mail.header[:Cc]}\n"
      out << "Date: #{mail.date.to_s}\n"
      out << "</pre>"
      out << html_part
      out << "</body></html>"
      out << html_part
      puts html_part
    end

    out.rewind

    vym_mgr = VymManager.new
    vym = vym_mgr.find('production') 
    if !vym
      puts "Couldn't find instance name \"#{instance_name}\", please start one:"
      puts "vym -l -n \"#{instance-name}\" -t test/default.vym"
      exit
    end

    mapNumber = vym.currentModel
    map = vym.map(1)

    # Before doing anything, make sure there is a return value available
    # Otherwise the script might block     // FIXME-1
    vym.version

    map.addBranch()
    map.selectLatestAdded
    map.setHeadingPlainText(mail.subject)
    map.loadNote(out.path)
  ensure
    out.close
    out.unlink
  end

ensure
  mail_in.close
  mail_in.unlink
end
