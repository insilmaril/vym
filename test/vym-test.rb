#!/usr/bin/env ruby

require "#{ENV['PWD']}/scripts/vym-ruby"
require 'date'
require 'optparse'

instance_name = 'test'

options = {}
OptionParser.new do |opts|
  opts.banner = "Usage: vym-test.rb [options]"

  opts.on('-d', '--directory  NAME', 'Directory name') { |s| options[:testdir] = s }
end.parse!

@testdir = options[:testdir]

$tests_passed = 0
$tests_failed = 0
$tests_total  = 0

def waitkey
  puts "Press return to continue..."
  STDIN.gets
end

def expect (comment, v_real, v_exp)
  if v_exp == v_real
    puts "    Ok: #{comment}"
    $tests_passed += 1
  else  
    puts "Failed: #{comment}. Expected '#{v_exp}', but got '#{v_real}'"
    $tests_failed += 1
    waitkey
  end  
  $tests_total += 1
end    

def expect_error (comment, error)
  if error.length ==  0
    puts "Failed: #{comment}. Command did not return error."
    $tests_failed += 1
  else  
    puts "    Ok: #{comment}"
    $tests_passed += 1
  end
  $tests_total += 1
end

def heading (s)
  puts "\n#{s}\n#{'-' * s.length}\n"
end

def init_map
  # FIXME-2 Missing: check or init default map 
  # Map Structure:
  # MapCenter 0
  #   Main A
  #     branch a
  #       branch a1
  #       branch a2
  #       branch a3
  #   Main B
  # MapCenter 1
end

def summary
  puts "\nTests done  : #{$tests_total}"
  puts "Tests passed: #{$tests_passed}"
  puts "Tests failed: #{$tests_failed}"
end

vym_mgr = VymManager.new
#vym_mgr.show_running

vym = vym_mgr.find(instance_name)
if !vym
  puts "Couldn't find instance name \"#{instance_name}\", please start one:"
  puts "vym -l -n \"#{instance-name}\" -t test/default.vym"
  exit
end

#######################
@center_0="mc:0"
@center_1="mc:1"
@main_a="mc:0,bo:0"
  @branch_a=@main_a+",bo:0"
  @branch_b=@main_a+",bo:1"
  @branch_c=@main_a+",bo:2"
@main_b="mc:0,bo:1"

@n_centers=2

#######################
def test_vym (vym)
  heading "Mainwindow checks:"
  version = "2.5.2"
  expect "Version is #{version}", vym.version, version

  n = vym.mapCount.to_i
  mapname = "test/default.vym"
  vym.loadMap (mapname)
  expect "mapCount increased after loading \"#{mapname}\"", vym.mapCount.to_i, n + 1

  map = vym.map(1)

  title = "vym map used for testing"
  expect "map title is '#{title}'", map.getMapTitle, title
  author ="Uwe Drechsel"
  expect "Author is '#{author}'", map.getMapAuthor, author
end

#######################
def test_basics (vym)
  heading "Basic checks:"
  init_map
  vym.select @main_a
  expect "select mainbranch A", vym.getSelectString, @main_a
  expect "getHeadingPlainText", vym.getHeadingPlainText, "Main A"
  expect "branchCount", vym.branchCount, 3

  vym.selectLastBranch
  expect "selectLastBranch", vym.getHeadingPlainText, "branch c"

  vym.selectParent
  expect "selectParent", vym.getHeadingPlainText, "Main A"

  expect "getDestPath: Got #{vym.getDestPath}", vym.getDestPath, @testdir + "/testmap.vym" 
  expect "getFileDir:  Got #{vym.getFileDir}", vym.getFileDir, @testdir + "/" 
end

#######################
def test_export (vym)
  heading "Export:"
  init_map

  #HTML
  mapname = "export-html"
  htmlpath = "#{@testdir}/#{mapname}.html"
  flagpath = "#{@testdir}/flags/flag-stopsign.png"
  pngpath = "#{@testdir}/#{mapname}.png"
  csspath = "#{@testdir}/vym.css"
  vym.exportHTML(@testdir,htmlpath)
  expect "exportHTML: HTML file exists", File.exists?(htmlpath), true
  expect "exportHTML: HTML image exists", File.exists?(pngpath), true
  expect "exportHTML: HTML flags exists", File.exists?(flagpath), true
  expect "exportHTML: HTML CSS exists", File.exists?(csspath), true
  File.delete(htmlpath)
  File.delete(flagpath)
  File.delete(pngpath)
  File.delete(csspath)
  vym.exportLast
  expect "exportLast: HTML file exists", File.exists?(htmlpath), true
  expect "exportLast: HTML image exists", File.exists?(pngpath), true
  expect "exportLast: HTML flags exists", File.exists?(flagpath), true
  expect "exportLast: HTML CSS exists", File.exists?(csspath), true

  #AO
  filepath = "#{@testdir}/export-ao.txt"
  vym.exportAO(filepath)
  expect "exportAO:    AO file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:  AO file exists", File.exists?(filepath), true

  #ASCII
  filepath = "#{@testdir}/export-ascii.txt"
  vym.exportASCII(filepath,false)
  expect "exportASCII: ASCII file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:  ASCII file exists", File.exists?(filepath), true

  #CSV
  filepath = "#{@testdir}/export-csv.txt"
  vym.exportCSV(filepath)
  expect "exportCSV:    CSV file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:  CSV file exists", File.exists?(filepath), true

  #Image
  filepath = "#{@testdir}/export-image.png"
  vym.exportImage(filepath,"PNG")
  expect "exportImage: PNG file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:  PNG file exists", File.exists?(filepath), true

  #LaTeX
  filepath = "#{@testdir}/export-LaTeX.tex"
  vym.exportLaTeX(filepath)
  expect "exportLaTeX:  LaTeX file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:   LaTeX file exists", File.exists?(filepath), true

  #OrgMode
  filepath = "#{@testdir}/export-orgmode.org"
  vym.exportOrgMode(filepath)
  expect "exportOrgMode:  OrgMode file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast:     OrgMode file exists", File.exists?(filepath), true

  #PDF
  filepath = "#{@testdir}/export-pdf.pdf"
  vym.exportPDF(filepath)
  expect "exportPDF:  PDF file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast: PDF file exists", File.exists?(filepath), true

  #SVG
  filepath = "#{@testdir}/export-svg.svg"
  vym.exportSVG(filepath)
  expect "exportSVG:  SVG file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast: SVG file exists", File.exists?(filepath), true

  #XML
  filepath = "#{@testdir}/export-xml.xml"
  vym.exportXML(@testdir, filepath)
  expect "exportXML: XML file exists", File.exists?(filepath), true
  File.delete(filepath)
  vym.exportLast
  expect "exportLast: XML file exists", File.exists?(filepath), true

  #OpenOffice Impress //FIXME-2
  #KDE4 Bookmarks //FIXME-2
  #Taskjuggler //FIXME-3
end

#######################
def test_extrainfo (vym)
  heading "Extra information:"
  init_map
  vym.setMapAuthor("Fra Erasmas")
  expect "Set and get map author", vym.getMapAuthor, "Fra Erasmas"
  vym.setMapComment("xy z")
  expect "Set and get map comment", vym.getMapComment, "xy z"
  vym.setMapTitle("vym rules!")
  expect "Set and get map title", vym.getMapTitle, "vym rules!"
end

#######################
def test_adding_branches (vym)
  heading "Adding branches:"
  init_map
  vym.select @main_a
  n=vym.branchCount
  vym.addBranch()
  expect( "addBranch", vym.branchCount, n+1 )

  vym.selectLatestAdded
  expect "selectLatestAdded", vym.getSelectString, @main_a+",bo:3"

  vym.undo
  expect( "Undo: addBranch", vym.branchCount, n )

  init_map
  vym.select @main_a
  n=vym.branchCount
  vym.select @branch_a
  vym.addBranch(-3)
  vym.addBranch(-1)
  vym.select @main_a
  expect "addBranchAbove/Below", vym.branchCount, n+2

  vym.undo
  vym.undo
  expect "Undo: addBranchAbove/Below", vym.branchCount, n

  init_map
  vym.select @branch_a
  vym.addBranchBefore
  vym.select @main_a
  expect "addBranchBefore: check branchcount",  vym.branchCount, n
  vym.select @branch_a
  expect "addBranchBefore: check heading", vym.getHeadingPlainText, ""
  vym.undo
  vym.select @main_a
  expect "Undo: addBranchBefore", vym.branchCount, n  
end

#######################
def test_adding_maps (vym)
  heading "Adding maps"
  init_map
  vym.select @branch_a
  n=vym.branchCount
  vym.addMapReplace "test/default.vym"
  vym.select @main_a
  expect "addMapReplace: check branch count in #{@main_a}", vym.branchCount, n + @n_centers -1
  vym.select @branch_a
  expect "addMapReplace: check if #{@branch_a} is new", vym.branchCount, 2
  
  vym.undo
  vym.select @main_a
  expect "Undo: check branch count in #{@main_a}", vym.branchCount, 3
  vym.select @branch_a
  expect "Undo: check if #{@branch_a} is back", vym.branchCount, 3 
  
  init_map
  vym.select @main_a
  vym.addMapInsert "test/default.vym",1
  vym.select @main_a
  expect "addMapInsert: branch count",  vym.branchCount, n+2 
  vym.select @main_a + ",bo:1"
  expect "addMapInsert: new heading", vym.getHeadingPlainText, "MapCenter 0"
  
  vym.undo
  vym.select @main_a
  expect "Undo: check branch count in #{@main_a}", vym.branchCount, 3 
  vym.select @branch_b
  expect "Undo: check heading of  #{@branch_b}",  vym.getHeadingPlainText, "branch b"
end

#######################
def test_scrolling (vym)
  heading "Scrolling and unscrolling"
  init_map
  vym.select @main_a
  vym.toggleScroll
  expect "toggleScroll", vym.isScrolled, true
  vym.undo
  expect "undo toggleScroll", vym.isScrolled, false
  vym.scroll
  expect "scroll", vym.isScrolled, true
  vym.unscroll
  expect "unscroll", vym.isScrolled, false

  init_map
  vym.scroll
  vym.select @branch_a
  vym.scroll
  vym.select @main_a
  vym.unscrollChildren
  vym.select @branch_a
  expect "unscrollChildren", vym.isScrolled, false
  vym.undo
  expect "undo unscrollChildren", vym.isScrolled, true
  vym.unscroll
  vym.select @branch_a
  vym.unscroll
end

#######################
def test_moving_parts (vym)
  heading "Moving parts"
  init_map
  vym.select @branch_a
  vym.moveDown
  vym.select @branch_a
  expect "Moving down", vym.getHeadingPlainText, "branch b"
  vym.undo
  vym.select @branch_a
  expect "Undo Moving down", vym.getHeadingPlainText, "branch a"
  
  init_map
  vym.select @branch_b
  vym.moveUp
  vym.select @branch_a
  expect "Moving up", vym.getHeadingPlainText, "branch b"
  vym.undo
  vym.select @branch_b
  expect "Undo Moving up", vym.getHeadingPlainText, "branch b"
  
  init_map
  vym.select @main_b
  n=vym.branchCount
  vym.select @branch_a
  vym.relinkTo @main_b,0,0,0
  vym.select @main_b
  expect "RelinkTo #{@main_b}: branchCount increased there",  vym.branchCount, n+1

  vym.undo
  vym.select @branch_b
  expect "Undo: RelinkTo #{@main_b}: branchCount decreased there", vym.branchCount, n
  
  init_map
  vym.select @main_a
  err = vym.relinkTo @branch_a,0,0,0
  expect_error "RelinkTo myself fails.", err

  vym
  init_map
  vym.select @branch_a
  n=vym.branchCount
  vym.select @main_b
  vym.relinkTo @branch_a, 1, 0, 0
  vym.select @branch_a
  expect "RelinkTo #{@branch_a}, pos 1: branchCount increased there",  vym.branchCount, n+1
  vym.select "#{@branch_a},bo:1"
  expect "RelinkTo #{@branch_a}, pos 1: Mainbranch really moved", vym.getHeadingPlainText, "Main B"
  vym.undo
  vym.select @center_0
  expect "Undo RelinkTo pos 1: branchCount of center", vym.branchCount, 2
  # FIXME-2 still has wrong position, check position
  vym.select @main_b
  vym.moveRel 100,100
end

#######################
def test_modify_branches (vym)
  heading "Modifying branches"
  init_map
  vym.select @branch_a
  vym.setHeadingPlainText "Changed!"
  expect "setHeadingPlainText", vym.getHeadingPlainText, "Changed!"
  vym.undo
  expect "Undo: setHeadingPlainText", vym.getHeadingPlainText, "branch a"
  vym.redo
  expect "redo: setHeadingPlainText", vym.getHeadingPlainText, "Changed!" 
  vym.undo
end  
  
#######################
def test_flags (vym)
  heading "Flags"
  init_map
  vym.select @main_a
  
  def set_flags (v,a)
    a.each do |f|
    v.setFlag f
    expect "Flag set: #{f}", v.hasActiveFlag(f), true
    end
  end
  
  def unset_flags (v,a)
    a.each do |f|
    v.unsetFlag f
    expect "Flag unset: #{f}", v.hasActiveFlag(f), false
    end
  end
  
  # Group standard-mark
  set_flags vym,[ "exclamationmark","questionmark"]
  
  # Group standard-status
  set_flags vym, [ "hook-green", 
    "wip", 
    "cross-red", 
    "stopsign" ]
  
  # Group standard-smiley
  smileys = [ "smiley-good",
      "smiley-sad",
      "smiley-omb" ]
  set_flags vym, smileys
  
  # Group standard-arrow
  set_flags vym, [ "arrow-up", 
    "arrow-down", 
    "2arrow-up", 
    "2arrow-down" ]
  
  # Group standard-thumb
  set_flags vym, [ "thumb-up", "thumb-down" ]
  
  # Without group
  set_flags vym, [ "clock",
    "phone",
    "lamp",
    "rose",
    "heart",
    "present",
    "flash",
    "info",
    "lifebelt" ]
  
  unset_flags vym, smileys
  
  vym.clearFlags
  expect "clearFlags cleared exclamationmark", vym.hasActiveFlag( "exclamationmark" ), false
  expect "clearFlags cleared smiley-good", vym.hasActiveFlag( "smiley-good" ), false
  
  vym.toggleFlag "lifebelt"
  expect "toggleFlag: flag activated", vym.hasActiveFlag("lifebelt"),true
  vym.toggleFlag "lifebelt"
  expect "toggleFlag: flag deactivated", vym.hasActiveFlag("lifebelt"),false
end

#######################
def test_delete_parts (vym)
  heading "Deleting parts"
  init_map
  vym.select @main_a
  n=vym.branchCount
  vym.select @branch_a
  m=vym.branchCount
  vym.delete
  vym.select @main_a
  expect "Delete branch: branchcount",  vym.branchCount, n-1
  vym.undo
  vym.select @main_a
  expect "Undo Delete branch: branchcount parent", vym.branchCount, n
  vym.select @branch_a
  expect "Undo Delete branch: branchcount restored branch", vym.branchCount, m
  
  init_map
  vym.select @branch_a
  n=vym.branchCount
  vym.deleteChildren
  vym.select @branch_a
  expect "deleteChildren: branchcount", vym.branchCount, 0
  vym.undo
  vym.select @branch_a
  expect "Undo: deleteChildren: branchcount", vym.branchCount, n
  
  init_map
  vym.select @main_a
  n=vym.branchCount
  vym.select @branch_a
  m=vym.branchCount
  vym.deleteKeepChildren
  vym.select @main_a
  expect "deleteKeepChildren: branchcount", vym.branchCount, n+m-1
  vym.undo
  vym.select @main_a
  expect "Undo: deleteKeepChildren: branchcount of parent", vym.branchCount, n
  vym.select @branch_a
  expect "Undo: deleteKeepChildren: branchcount of branch", vym.branchCount, m

  init_map
  n = vym.centerCount
  vym.select @center_1
  vym.delete
  expect "Delete mapCenter: number of centers decreased", vym.centerCount, n-1
  vym.undo
  expect "Undo Delete mapCenter: number of centers increased", vym.centerCount, n
end  

#######################
def test_copy_paste (vym)
  heading "Copy, cut & Paste"
  init_map
  vym.select @main_a
  n=vym.branchCount
  
  vym.copy
  vym.paste
  vym.selectLastBranch
  s=vym.getSelectString
  expect "Normal paste of branch, check heading of #{s}", vym.getHeadingPlainText, "Main A"
  
  vym.undo
  vym.select @main_a
  expect "Undo paste: branchCount of #{@main_a}", vym.branchCount, n
  
  vym.redo
  vym.select s
  expect "redo paste: check heading", vym.getHeadingPlainText, "Main A"
  
  vym.cut
  vym.select @main_a
  expect "cut: branchCount of #{@main_a}", vym.branchCount, n
  vym.paste
  vym.selectLastBranch
  s=vym.getSelectString
  expect "Normal paste of branch, check heading of #{s}", vym.getHeadingPlainText, "Main A"
  vym.cut 
end 

#######################
def test_references (vym)
  heading "References"
  init_map
  vym.select @main_a
  vym.setURL "www.insilmaril.de"
  expect "setURL:", vym.getURL, "www.insilmaril.de"
  vym.undo
  expect "undo setURL", vym.getURL, ""
  vym.redo
  expect "redo setURL", vym.getURL, "www.insilmaril.de"
  vym.setURL ""
  expect "setURL: unset URL with empty string", vym.getURL, ""
  
  vl="default.vym"
  vym.setVymLink vl
  s=vym.getVymLink
  expect "setVymLink returns absolute path", vym.getVymLink, vym.getFileDir + vl
  vym.undo
  expect "undo: setVymLink", vym.getVymLink, ""
  vym.redo
  expect "redo: setVymLink", vym.getVymLink, s
  vym.undo
end

#######################
def test_history (vym)
  heading "History"
  init_map
  vym.select @main_a
  vym.setHeadingPlainText "A"
  vym.setHeadingPlainText "B"
  vym.setHeadingPlainText "C"
  vym.undo
  vym.undo
  vym.undo
  expect "Undo 3 times", vym.getHeadingPlainText, "Main A"
  vym.redo
  expect "Redo once", vym.getHeadingPlainText, "A"
  vym.copy
  vym.redo
  expect "Redo once more", vym.getHeadingPlainText, "B"
  vym.redo
  expect "Redo yet again", vym.getHeadingPlainText, "C"
  vym.setHeadingPlainText "Main A"
  vym.paste
  vym.selectLastBranch
  expect "Paste from the past", vym.getHeadingPlainText, "A"
  vym.delete
end  

#######################
def test_xlinks (vym)
  heading "XLinks:"
  init_map
  vym.addXLink("mc:0,bo:0","mc:0,bo:1",2,"#ff0000","Qt::DashDotLine")
  vym.selectLatestAdded
  expect "Default color of XLink", vym.getXLinkColor, "#ff0000"
  expect "Default width of XLink", vym.getXLinkWidth, 2
  expect "Default style of XLink", vym.getXLinkPenStyle, "Qt::DashDotLine"
  expect "Default style of XLink begin", vym.getXLinkStyleBegin, "HeadFull"
  expect "Default style of XLink end",   vym.getXLinkStyleEnd, "HeadFull"

  vym.setXLinkWidth(3)
  expect "New width of XLink", vym.getXLinkWidth, 3
  vym.undo
  expect "Undo width of XLink", vym.getXLinkWidth, 2

  vym.setXLinkColor("#00ff00")
  expect "New color of XLink", vym.getXLinkColor, "#00ff00"
  vym.undo
  expect "Undo color of XLink", vym.getXLinkColor, "#ff0000"

  vym.setXLinkLineStyle("Qt::SolidLine")
  expect "New style of XLink", vym.getXLinkPenStyle, "Qt::SolidLine"
  vym.undo
  expect "Undo style of XLink", vym.getXLinkPenStyle, "Qt::DashDotLine"

  vym.setXLinkStyleBegin("None")
  expect "New style of XLink begin", vym.getXLinkStyleBegin, "None"
  vym.undo
  expect "Undo style of XLink begin", vym.getXLinkStyleBegin, "HeadFull"

  vym.setXLinkStyleEnd("None")
  expect "New style of XLink end", vym.getXLinkStyleEnd, "None"
  vym.undo
  expect "Undo style of XLink end", vym.getXLinkStyleEnd, "HeadFull"

  vym.delete
end

#######################
def test_tasks (vym)
  heading "Tasks:"
  init_map
  vym.select @main_a
  expect "Branch has no task before test", vym.hasTask, false
  vym.toggleTask
  expect "Toggle task", vym.hasTask, true
  expect "Setting sleep days to 10", vym.setTaskSleep(10), true
  expect "Task sleep when setting to integer", vym.getTaskSleepDays, 10

  date_today = DateTime.now
  date_later = date_today + 123
  date_s = date_later.strftime("%Y-%m-%d") 
  vym.setTaskSleep(date_s)
  expect "Task sleep when setting to ISO date (#{date_s})", vym.getTaskSleepDays, 123
end

######################
def test_notes (vym)
  heading "Notes:"

  # Plaintext notes basic actions 
  init_map
  vym.select @main_a
  note_plain = "vymnote plaintext"
  vym.setNotePlainText(note_plain)
  expect "Set note to \"#{note_plain}\". Still plaintext?", vym.hasRichTextNote, false
  vym.select @center_0
  vym.select @main_a
  expect "After reselect, is note plaintext?", vym.hasRichTextNote, false

  note_plain = "<b>plaintext, not bold!</b>"
  vym.setNotePlainText(note_plain)
  expect "Set note to plaintext containing html tags. Still plaintext", vym.hasRichTextNote, false
  note_new = vym.getNotePlainText
  vym.select @center_0
  vym.select @main_a
  expect "After reselect, is note text unchanged?", vym.getNotePlainText, note_new
  expect "After reselect, is note plaintext?", vym.hasRichTextNote, false

  # Plaintext notes copy & paste
  vym.copy
  vym.paste
  vym.selectLastBranch
  s=vym.getSelectString
  expect "After copy& paste: New note unchanged?", vym.getNotePlainText, note_plain
  expect "After copy& paste: New note Still plaintext?", vym.hasRichTextNote, false
  vym.delete

  # Plaintext notes undo & redo
  vym.select @main_a
  vym.setNotePlainText('Foobar')
  vym.undo
  expect "Undo after setNotePlainText restores previous note", vym.getNotePlainText, note_plain
  vym.redo
  vym.select @main_a
  expect "Redo restores previous note", vym.getNotePlainText, 'Foobar'

  # Plaintext notes load & save
  note_org = IO.read('test/note-plain.txt')
  vym.loadNote("test/note-plain.txt") 
  expect "Load plain text note from file. Still plaintext?", vym.hasRichTextNote, false
  expect "Note contains 'not bold'", vym.getNotePlainText.include?("not bold"), true
  filepath = "#{@testdir}/save-note.txt"
  vym.saveNote(filepath)
  expect "Save note to file. Check if it contains 'textMode=\"plainText\"'", IO.read(filepath).include?("textMode=\"plainText\""), true
  expect "Save note to file. Check if it contains 'not bold'", IO.read(filepath).include?("not bold"), true
  expect "Save note to file. Check if it contains '<b>' element", IO.read(filepath).include?("<b>"), true
  expect "Save note to file. Check if it contains '<![CDATA['", IO.read(filepath).include?("<![CDATA["), true
  
  # Delete note
  vym.setNotePlainText("")
  expect "setNotePlainText(\"\") deletes note", vym.hasNote, false
  
  # RichText basic actions
  init_map
  vym.select @main_a
  rt_note = '<vymnote  textMode="richText"><![CDATA[<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="text/css"> p, li { white-space: pre-wrap; } </style></head><body style=" font-family:\'Arial\'; font-size:12pt; font-weight:400; font-style:normal;"> <p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:\'DejaVu Sans Mono\'; color:#000000;">Rich Text note with <b>not bold text</b></span></p></body></html>]]></vymnote>'
  vym.parseVymText(rt_note)
  expect "parseVymText of richText note produces note", vym.hasNote, true
  expect "parseVymText of richText note produces richText note", vym.hasRichTextNote, true
  vym.select @center_0
  vym.select @main_a
  expect "After reselect, is note RichText?", vym.hasRichTextNote, true


  # RichText notes copy & paste
  rt_note = vym.getNoteXML  
  vym.copy
  vym.paste
  vym.selectLastBranch
  s=vym.getSelectString
  expect "After copy& paste: New note Still RichText?", vym.hasRichTextNote, true
  expect "After copy& paste: New note unchanged?", vym.getNoteXML, rt_note
  vym.delete

  # RichText notes undo & redo
  vym.select @main_a
  vym.setNotePlainText('Foobar')
  vym.undo
  expect "Undo after setNotePlainText restores RichText note", vym.getNoteXML, rt_note
  vym.redo
  vym.select @main_a
  expect "Redo restores previous plaintext note", vym.getNotePlainText, 'Foobar'

  # RichText notes load & save
  vym.loadNote("test/note.html") 
  expect "Load HTML note from file and try to detect textMode. Is RichText?", vym.hasRichTextNote, true
  filepath = "#{@testdir}/save-note.txt"
  vym.saveNote(filepath)
  expect "Save note to file. Check if it contains 'textMode=\"richText\"'", IO.read(filepath).include?("textMode=\"richText\""), true
  expect "Save note to file. Check if it contains 'bold'", IO.read(filepath).include?("bold"), true
  expect "Save note to file. Check if it contains '<b>' element", IO.read(filepath).include?("<b>"), true
  expect "Save note to file. Check if it contains '<![CDATA['", IO.read(filepath).include?("<![CDATA["), true
  
  # Delete note
  vym.setNotePlainText("")
  expect "setNotePlainText(\"\") deletes note", vym.hasNote, false

  # Compatibility with version < 2.5.0  # FIXME missing
end

def test_headings (vym)
  heading "Headings:"
  # FIXME same checks like for notes above for headings
end
  
######################
def test_bugfixes (vym)
  heading "Bugfixes:"
  init_map
  vym.select @main_b
  expect "Mapcenter of #{@center_1} has no frame", vym.getFrameType, "NoFrame"
end

#######################
test_vym(vym)
#test_basics(vym)
#test_export(vym)
#test_extrainfo(vym)
#test_adding_branches(vym)
#test_adding_maps(vym)
#test_scrolling(vym)
#test_moving_parts(vym)
#test_modify_branches(vym)
#test_flags(vym)
#test_delete_parts(vym)
#test_copy_paste(vym)
#test_references(vym)
#test_history(vym)
#test_xlinks(vym)
#test_tasks(vym)
#test_notes(vym)
#test_headings(vym)
#test_bugfixes(vym)
summary

=begin
# Untested commands:
#
addSlide
centerOnID
colorBranch
colorSubtree
cycleTask
delete (image)
deleteSlide
importDir
loadImage
loadNote
move
moveRel
moveSlideDown
moveSlideUp
note2URLs
    paste
redo  
relinkTo (for images)
saveImage
saveNote
selectID
selectLastImage
selectLatestAdd
setFrameBorderWidth
setFrameBrushColor
setFrameIncludeChildren
setFramePadding
setFramePenColor
setFrameType
    setHeading
setHideExport
setHideLinksUnselected
setIncludeImagesHorizontally
setIncludeImagesVertically
setMapAnimCurve
setMapAnimDuration
setMapBackgroundColor
setMapDefLinkColor
setMapLinkStyle
setMapRotation
setMapZoom
setNote
setScale
setSelectionColor
setTaskSleep
    setURL
    setVymLink
  so far:
sortChildren
toggleFrameIncludeChildren
toggleTarget
toggleTask
=end
