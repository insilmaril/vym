#!/usr/bin/ruby

load File.expand_path("../vym-ruby.rb", __FILE__)
require 'tempfile'
require 'mail'

mail_in = Tempfile.new("mail")
begin
  mail_in.write(ARGF.read)
  mail_in.rewind
  out = Tempfile.new("temp")
  begin
    mail = Mail.read(mail_in.path)

    out <<  "Subject: #{mail.subject}\n"
    out <<  "From: #{mail.header[:From]}\n"
    out <<  "To: #{mail.header[:To]}\n"
    out <<  "Cc: #{mail.header[:Cc]}\n"
    out <<  "Date: #{mail.date.to_s}\n"
    out <<  "\n"
    out <<  mail.decoded

    out.rewind

    vym_mgr = VymManager.new
    vym = vym_mgr.find('production') 

    vym.addBranch 
    vym.selectLastBranch 
    vym.setHeading(mail.subject)
    vym.loadNote(out.path)
  ensure
    out.close
    out.unlink
  end

ensure
  mail_in.close
  mail_in.unlink
end
