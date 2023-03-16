#!/usr/bin/ruby

# This ruby script can be called from mutt to create a branch in vym
# with the mail currently visible in mutts pager.
# The mail itself will be added as note to a branch, which has the
# heading "YYYY-MM-DD Email: $SUBJECT"
#
# vym is notified using DBUS, which usually only is available on Linux
# systems
#
# Example entry for .muttrc:
# macro pager Y "|/home/uwe/vym/branches/develop/scripts/vym-addmail.rb\n" "Add note to production inst ance of vym"
#
# vym itself needs to be starting using the name option, using  "vym -n production"
#

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

    text = ""
    if mail.multipart? then
      puts "Warning: multipart mail detected. Only using first part."
      text = mail.parts.first.decoded
    else
      #out <<  mail.body.raw_source.gsub("\n", "<br/>")
      text = mail.decoded
    end

    if text.include?("<html")
      out << text
    else
      out << "<p>"
      text.gsub!("\r\n\r\n", "</p><p>")
      text.gsub!("\r\n", "")
      out << text
      out << "</p>"
    end

    out << "</body></html>"

    out.rewind

    name = "production"
    #name = "test"

    vym_mgr = VymManager.new
    vym_mgr.show_running
    vym = vym_mgr.find(name)

    if !vym
      puts "Couldn't find instance named \"#{name}\", please start one:"
      puts "vym -l -n #{name} -t test/default.vym"

      puts "Currently running vym instances: #{vym_mgr.show_running}"
      exit
    end

    # Before doing anything, make sure there is a return value available
    # Otherwise the script might block     // FIXME
    #version = vym.version

    puts "Found #{vym.mapCount} maps"

    #puts vym.show_methods

    n = vym.currentMapID()
    puts "ID of current map: #{n}"

    map = vym.map (n)
    puts "Map title: #{map.getMapTitle}"
    puts "Map path:  #{map.getDestPath}"
    puts "Map name:  #{map.getFileName}"

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
