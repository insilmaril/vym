#!/usr/bin/env ruby

require "#{ENV['PWD']}/../scripts/vym-ruby"
require 'colorize'
require 'date'
require 'fileutils'
require 'optparse'

def waitkey
  puts "Press return to continue..."
  STDIN.gets
end

def expect (comment, v_real, v_exp)
  if v_exp == v_real
    puts "    Ok: #{comment}".green
    $tests_passed += 1
    # waitkey
  else
    puts "Failed: #{comment}. Expected '#{v_exp}', but got '#{v_real}'".red
    $tests_failed += 1
    waitkey
  end
  $tests_total += 1
end

def expectNot (comment, v_real, v_exp)
  if v_exp != v_real
    puts "    Ok: #{comment}".green
    $tests_passed += 1
    # waitkey
  else
    puts "Failed: #{comment}. Expected not to get '#{v_exp}'".red
    $tests_failed += 1
    waitkey
  end
  $tests_total += 1
end

def expect_warning_only (comment, v_real, v_exp)
  if v_exp == v_real
    puts "    Ok: #{comment}".green
    $tests_passed += 1
    # waitkey
  else
    puts "Warning: #{comment}. Expected '#{v_exp}', but got '#{v_real}'".red
    $tests_warnings += 1
  end
  $tests_total += 1
end

def expect_error (comment, error)
  if error.length ==  0
    puts "Failed: #{comment}. Command did not return error.".red
    $tests_failed += 1
  else
    puts "    Ok: #{comment}".green
    $tests_passed += 1
  end
  $tests_total += 1
end

def heading (s)
  puts "\n#{s}\n#{'-' * s.length}\n".yellow
end

def init_map( mapPath )
  # Copy the map referenced above to @testDir/test-current.[vym|xml]
  # and try to load it
  @currentMapPath = "#{@testDir}/test-current#{File.extname(mapPath)}"

  begin
    puts "init_map #{mapPath}"
    FileUtils.cp mapPath, @currentMapPath
  rescue
    puts "Failed to copy #{mapPath} to #{@currentMapPath}"
    exit
  end

  if @vym.loadMap (@currentMapPath)
    puts "# Loaded #{mapPath} -> #{@currentMapPath}".blue
    id = @vym.currentMapID
    return @vym.map (id)
  end

  puts "Failed to load #{mapPath}".red
  exit
end

def close_current_map
  id = @vym.currentMapID
  r = @vym.closeMapWithID(id)
  if r
    puts "# Closed map with id = #{id}".blue
  else
    puts "# Failed to close map with id = #{id}. CurrentMapID = #{id}".red
  end
end

def summary
  puts "\nTests done  : #{$tests_total}"
  puts "Tests passed: #{$tests_passed}"
  puts "Warnings:     #{$tests_warnings}"
  puts "Tests failed: #{$tests_failed}"
end

#######################
def test_vym
  #@vym.clearConsole

  heading "Mainwindow checks:"
  version = "2.9.510"
  expect_warning_only "Version is #{version}", @vym.version, version

  expect "Temporary directory exists at '#{@testDir}'", File.exists?(@testDir), true


  map = init_map @testMapDefault
  expect "init_map copies default testmap to '#{@currentMapPath}'", File.file?(@currentMapPath), true
  expect "Title of copied map title is accessible and not empty", map.getMapTitle.length > 0, true

  close_current_map
end

#######################
def test_basics
  heading "Basic checks:"
  map = init_map @testMapDefault

  title = "vym map used for testing"
  expect "map title is '#{title}'", map.getMapTitle, title
  author ="Uwe Drechsel"
  expect "Author is '#{author}'", map.getMapAuthor, author

  map.select @main_a
  expect "select mainbranch A", map.getSelectionString, @main_a
  expect "getHeadingPlainText", map.getHeadingPlainText, "Main A"
  expect "branchCount", map.branchCount, 3

  map.selectLastBranch
  expect "selectLastBranch", map.getHeadingPlainText, "Main B"

  map.selectFirstBranch
  expect "selectFirstBranch", map.getHeadingPlainText, "Main A"

  map.selectParent
  expect "selectParent", map.getHeadingPlainText, "MapCenter 0"

  expect "getDestPath: Got #{map.getDestPath}", map.getDestPath, @testDir + "/test-current.vym"
  expect "getFileDir:  Got #{map.getFileDir}", map.getFileDir, @testDir + "/"

  close_current_map
end

#######################
def test_adding_branches
  heading "Adding branches:"
  map = init_map @testMapDefault
  map.select @main_a
  n = map.branchCount.to_i
  map.addBranch()
  expect( "addBranch", map.branchCount.to_i, n + 1 )

  map.selectLatestAdded
  expect "selectLatestAdded", map.getSelectionString, @main_a + ",bo:3"

  map.undo
  expect( "Undo: addBranch", map.branchCount.to_i, n )

  close_current_map
  map = init_map @testMapDefault

  map.select @main_a
  n = map.branchCount.to_i
  map.select @branch_a
  map.addBranch( -3 )
  map.addBranch( -1 )
  map.select @main_a
  expect "addBranchAbove/Below", map.branchCount.to_i, n + 2

  map.undo
  map.undo
  expect "Undo: addBranchAbove/Below", map.branchCount.to_i, n

  close_current_map
  map = init_map @testMapDefault

  map.select @branch_a
  map.addBranchBefore
  map.select @main_a
  expect "addBranchBefore: check branchcount",  map.branchCount.to_i, n
  map.select @branch_a
  expect "addBranchBefore: check heading", map.getHeadingPlainText, ""

  # Undo twice: addBranchNew and relinkTo
  map.undo
  map.undo
  map.select @main_a
  expect "Undo: addBranchBefore", map.branchCount.to_i, n

  close_current_map
end

#######################
def test_adding_maps
  heading "Adding maps"
  map = init_map @testMapDefault
  map.select @branch_a
  n = map.branchCount.to_i
  map.addMapReplace @currentMapPath
  map.select @main_a
  expect "addMapReplace: check branch count in #{@main_a}", map.branchCount.to_i, n + 1
  map.select @branch_a
  expect "addMapReplace: check if #{@branch_a} is new", map.branchCount.to_i, 2
  expect "addMapReplace: Loaded MapCenter 0", map.getHeadingPlainText, "MapCenter 0"
  map.select @branch_b
  expect "addMapReplace: Loaded MapCenter 1", map.getHeadingPlainText, "MapCenter 1"

  map.undo
  map.select @main_a
  expect "Undo: check branch count in #{@main_a}", map.branchCount.to_i, 3
  map.select @branch_a
  expect "Undo: check if #{@branch_a} is back", map.branchCount.to_i, 3
  close_current_map

  map = init_map @testMapDefault
  map.select @branch_a
  n = map.branchCount.to_i
  map.addMapInsert @currentMapPath, 1  # Create testmap with several MCs
  map.select @branch_a
  expect "addMapInsert: branch count",  map.branchCount.to_i, n + 2
  map.select @branch_a + ",bo:1"
  expect "addMapInsert: new heading", map.getHeadingPlainText, "MapCenter 0"
  map.select @branch_a + ",bo:2"
  expect "addMapInsert: new heading", map.getHeadingPlainText, "MapCenter 1"

  map.undo
  map.select @branch_a
  expect "Undo: check branch count in #{@branch_a}", map.branchCount.to_i, 3
  map.select @branch_b
  expect "Undo: check heading of  #{@branch_b}",  map.getHeadingPlainText, "branch b"
  close_current_map
end

#######################
def test_attributes
  heading "Attributes:"
  map = init_map "maps/test-attributes.xml"

  map.select @main_a
  expect "String attribute is '6 * 9'", map.getStringAttribute("string-attribute"), "6 * 9"
  expect "Integer attribute is 42", map.getIntAttribute("int-attribute"), 42

  close_current_map
end

######################
def test_bugfixes
  heading "Bugfixes:"
  map = init_map @testMapDefault

  close_current_map
end

#######################
def test_copy_paste
  heading "Copy, cut & Paste"

  map = init_map @testMapDefault
  map.select @main_a
  n = map.branchCount.to_i

  map.copy
  map.paste
  map.selectLatestAdded     #FIXME-2 not set for ImportAdd, which is used by paste
  s = map.getSelectionString
  expect "Normal paste of branch, check heading of #{s}", map.getHeadingPlainText, "Main A"

  map.undo
  map.select @main_a
  expect "Undo paste: branchCount of #{@main_a}", map.branchCount.to_i, n

  map.redo
  map.select s
  expect "redo paste: check heading", map.getHeadingPlainText, "Main A"

  map.select @branch_a
  map.cut
  map.select @main_a
  expect "cut: branchCount of #{@main_a}", map.branchCount.to_i, n

  map.paste
  map.selectLastChildBranch
  s = map.getSelectionString
  expect "Normal paste of branch, check heading of #{s}", map.getHeadingPlainText, "branch a"
  map.cut

  close_current_map
end

#######################
def test_delete_parts
  heading "Deleting parts"

  map = init_map @testMapDefault
  map.select @main_a
  n=map.branchCount.to_i
  map.select @branch_a
  m=map.branchCount.to_i
  map.remove
  map.select @main_a
  expect "Remove branch: branchcount",  map.branchCount.to_i, n - 1
  map.undo
  map.select @main_a
  expect "Undo Remove branch: branchcount parent", map.branchCount.to_i, n
  map.select @branch_a
  expect "Undo Remove branch: branchcount restored branch", map.branchCount.to_i, m

  close_current_map

  map = init_map @testMapDefault
  map.select @branch_a
  n = map.branchCount.to_i
  map.removeChildren
  map.select @branch_a
  expect "removeChildren: branchcount", map.branchCount.to_i, 0
  map.undo
  map.select @branch_a
  expect "Undo: removeChildren: branchcount", map.branchCount.to_i, n

  close_current_map
  map = init_map @testMapDefault

  map.select @main_a
  n=map.branchCount.to_i
  map.select @branch_a
  m=map.branchCount.to_i
  map.removeKeepChildren
  map.select @main_a
  expect "removeKeepChildren: branchcount", map.branchCount.to_i, n + m - 1
  map.undo
  map.select @main_a
  expect "Undo: removeKeepChildren: branchcount of parent", map.branchCount.to_i, n
  map.select @branch_a
  expect "Undo: removeKeepChildren: branchcount of branch", map.branchCount.to_i, m

  close_current_map
  map = init_map @testMapDefault

  n = map.centerCount.to_i
  map.select @center_1
  map.remove
  expect "remove mapCenter: number of centers decreased", map.centerCount.to_i, n - 1
  map.undo
  expect "Undo remove mapCenter: number of centers increased", map.centerCount.to_i, n

  close_current_map
end

#######################
def test_export
  heading "Export:"
  map = init_map @testMapDefault

  #HTML
  exportdir = "#{@testDir}/export-html"
  Dir.mkdir(exportdir)
  htmlpath = "#{exportdir}/output.html"
  flagdir  = "#{exportdir}/flags"
  pngpath = "#{exportdir}/output.png"
  csspath = "#{exportdir}/vym.css"
  map.exportMap("HTML", htmlpath, exportdir)
  expect "exportHTML: HTML file exists", File.exists?(htmlpath), true
  expect "exportHTML: HTML image exists", File.exists?(pngpath), true
  expect "exportHTML: HTML flags dir exists", Dir.exists?(flagdir), true
  if Dir.exists?(flagdir)
    expect "exportHTML: HTML flags dir not empty", Dir.empty?(flagdir), false
  end
  expect "exportHTML: HTML CSS exists", File.exists?(csspath), true
  File.delete(htmlpath)
  FileUtils.rm_r(flagdir)
  File.delete(pngpath)
  File.delete(csspath)
  map.exportMap("Last")
  expect "exportLast: HTML #{htmlpath} file exists", File.exists?(htmlpath), true
  expect "exportLast: HTML image exists", File.exists?(pngpath), true
  expect "exportHTML: HTML flags dir exists", Dir.exists?(flagdir), true
  if Dir.exists?(flagdir)
    expect "exportHTML: HTML flags dir not empty", Dir.empty?(flagdir), false
  end
  expect "exportLast: HTML CSS exists", File.exists?(csspath), true

  #AO
  exportdir = "#{@testDir}/export-ao"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.txt"
  map.exportMap("AO", filepath)
  expect "exportAO:    AO file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:  AO file exists", File.exists?(filepath), true

  #ASCII
  exportdir = "#{@testDir}/export-ascii"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.txt"
  map.exportMap("ASCII", filepath, false)
  expect "exportASCII: ASCII file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:  ASCII file exists", File.exists?(filepath), true

  #CSV
  exportdir = "#{@testDir}/export-csv"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.csv"
  map.exportMap("CSV", filepath)
  expect "exportCSV:    CSV file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:  CSV file exists", File.exists?(filepath), true

  #Image
  exportdir = "#{@testDir}/export-image"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.png"
  map.exportMap("Image", filepath,"PNG")
  expect "exportImage: PNG file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:  PNG file exists", File.exists?(filepath), true

  #LaTeX
  exportdir = "#{@testDir}/export-latex"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.tex"
  map.exportMap("LaTeX", filepath)
  expect "exportLaTeX:  LaTeX file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:   LaTeX file exists", File.exists?(filepath), true

  #Markdown
  exportdir = "#{@testDir}/export-markdown"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.md"
  map.exportMap("Markdown", filepath)
  expect "exportMarkdown:  Markdown file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:     Markdown file exists", File.exists?(filepath), true

  #OrgMode
  exportdir = "#{@testDir}/export-orgmode"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.org"
  map.exportMap("OrgMode", filepath)
  expect "exportOrgMode:  OrgMode file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast:     OrgMode file exists", File.exists?(filepath), true

  #PDF
  exportdir = "#{@testDir}/export-pdf"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.pdf"
  map.exportMap("PDF", filepath)
  expect "exportPDF:  PDF file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast: PDF file exists", File.exists?(filepath), true

  #SVG
  exportdir = "#{@testDir}/export-svg"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.svg"
  map.exportMap("SVG", filepath)
  expect "exportSVG:  SVG file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast: SVG file exists", File.exists?(filepath), true

  #XML
  exportdir = "#{@testDir}/export-xml"
  Dir.mkdir(exportdir)
  filepath = "#{exportdir}/output.xml"
  map.exportMap("XML", filepath, @testDir)
  expect "exportXML: XML file exists", File.exists?(filepath), true
  File.delete(filepath)
  map.exportMap("Last")
  expect "exportLast: XML file exists", File.exists?(filepath), true

  #OpenOffice Impress //FIXME-2
  #Taskjuggler //FIXME-3

  close_current_map
end

#######################
def test_extrainfo
  heading "Extra information:"
  map = init_map @testMapDefault
  map.setMapAuthor("Fra Erasmas")
  expect "Set and get map author", map.getMapAuthor, "Fra Erasmas"
  map.setMapComment("xy z")
  expect "Set and get map comment", map.getMapComment, "xy z"
  map.setMapTitle("vym rules!")
  expect "Set and get map title", map.getMapTitle, "vym rules!"

  close_current_map
end

######################
def test_frames
  heading "Frames:"
  map = init_map @testMapFrames

  map.select @center_0
  expect "Mapcenter of #{@center_0} has no inner frame", map.getFrameType(true), "NoFrame"
  expect "Mapcenter of #{@center_0} has no outer frame", map.getFrameType(true), "NoFrame"

  map.select @center_1
  expect "Mapcenter of #{@center_1} has no inner frame", map.getFrameType(true), "NoFrame"
  expectNot "Mapcenter of #{@center_1} has outer frame", map.getFrameType(false), "NoFrame"

  map.select @center_2
  expectNot "Mapcenter of #{@center_2} has inner frame", map.getFrameType(true), "NoFrame"
  expect "Mapcenter of #{@center_2} has no outer frame", map.getFrameType(false), "NoFrame"

  map.select @center_3
  expectNot "Mapcenter of #{@center_3} has inner frame", map.getFrameType(true), "NoFrame"
  expectNot "Mapcenter of #{@center_3} has outer frame", map.getFrameType(false), "NoFrame"
  close_current_map
end

def test_headings
  heading "Headings:"
  # FIXME same checks like for notes above for headings
end

#######################
def test_history
  heading "History"

  map = init_map @testMapDefault
  map.select @main_a
  map.setHeadingPlainText "A"
  map.setHeadingPlainText "B"
  map.setHeadingPlainText "C"
  map.undo
  map.undo
  map.undo
  expect "Undo 3 times, after changing heading -> 'A' -> 'B' -> 'C'", map.getHeadingPlainText, "Main A"
  map.redo
  expect "Redo once", map.getHeadingPlainText, "A"
  map.copy
  map.redo
  expect "Redo once more", map.getHeadingPlainText, "B"
  map.redo
  expect "Redo yet again", map.getHeadingPlainText, "C"
  map.setHeadingPlainText "Main A"
  map.paste
  map.selectLastChildBranch
  expect "Paste from the past", map.getHeadingPlainText, "A"
  map.remove

  close_current_map
end

#######################
def test_scrolling
  heading "Scrolling and unscrolling"
  map = init_map @testMapDefault

  map.select @main_a
  map.toggleScroll
  expect "toggleScroll", map.isScrolled, true
  map.undo
  expect "undo toggleScroll", map.isScrolled, false
  map.scroll
  expect "scroll", map.isScrolled, true
  map.unscroll
  expect "unscroll", map.isScrolled, false

  map.scroll
  map.select @branch_a
  map.scroll
  map.select @main_a
  map.unscrollChildren
  map.select @branch_a
  expect "unscrollChildren", map.isScrolled, false
  map.undo
  expect "undo unscrollChildren", map.isScrolled, true

  close_current_map
end

#######################
def test_slides
  heading "Slides"
  map = init_map "maps/test-slides.xml"

  map.select @main_a
  expect "Successfully loaded map with slides", map.slideCount, 3

  close_current_map
end

#######################
def test_modify_branches
  heading "Modifying branches"
  map = init_map @testMapDefault

  map.select @branch_a
  map.setHeadingPlainText "Changed!"
  expect "setHeadingPlainText", map.getHeadingPlainText, "Changed!"
  map.undo
  expect "Undo: setHeadingPlainText", map.getHeadingPlainText, "branch a"
  map.redo
  expect "redo: setHeadingPlainText", map.getHeadingPlainText, "Changed!"
  map.undo

  close_current_map
end

#######################
def test_moving_parts
  heading "Moving parts"
  map = init_map @testMapDefault

  map.select @branch_a
  map.moveDown
  map.select @branch_a
  expect "Moving down", map.getHeadingPlainText, "branch b"
  map.undo
  map.select @branch_a
  expect "Undo Moving down", map.getHeadingPlainText, "branch a"

  #map = init_map( vym )
  map.select @branch_b
  map.moveUp
  map.select @branch_a
  expect "Moving up", map.getHeadingPlainText, "branch b"
  map.undo
  map.select @branch_b
  expect "Undo Moving up", map.getHeadingPlainText, "branch b"

  #map = init_map( vym )
  map.select @main_b
  n=map.branchCount.to_i
  map.select @branch_a
  map.relinkTo @main_b,0,0,0
  map.select @main_b
  expect "RelinkTo #{@main_b}: branchCount increased there",  map.branchCount.to_i, n+1

  map.undo
  map.select @branch_b
  expect "Undo: RelinkTo #{@main_b}: branchCount decreased there", map.branchCount.to_i, n

  #map = init_map( vym )
  map.select @main_a
  err = map.relinkTo @branch_a,0,0,0
  #FIXME-2 disabled, error not supported atm expect_error "RelinkTo myself fails.", err

  #map = init_map( vym )
  map.select @branch_a
  n=map.branchCount.to_i
  map.select @main_b
  map.relinkTo @branch_a, 1, 0, 0
  map.select @branch_a
  expect "RelinkTo #{@branch_a}, pos 1: branchCount increased there",  map.branchCount.to_i, n+1
  map.select "#{@branch_a},bo:1"
  expect "RelinkTo #{@branch_a}, pos 1: Mainbranch really moved", map.getHeadingPlainText, "Main B"
  map.undo
  map.select @center_0
  expect "Undo RelinkTo pos 1: branchCount of center", map.branchCount.to_i, 2

  close_current_map
end

######################
def test_notes
  heading "Notes:"

  # Plaintext notes basic actions
  map = init_map @testMapDefault

  map.select @main_a
  note_plain = "vymnote plaintext"
  map.setNotePlainText(note_plain)
  expect "Set note to \"#{note_plain}\". Still plaintext?", map.hasRichTextNote, false
  map.select @center_0
  map.select @main_a
  expect "After reselect, is note plaintext?", map.hasRichTextNote, false

  note_plain = "<b>plaintext, not bold!</b>"
  map.setNotePlainText(note_plain)
  expect "Set note to plaintext containing html tags. Still plaintext", map.hasRichTextNote, false
  note_new = map.getNotePlainText
  map.select @center_0
  map.select @main_a
  expect "After reselect, is note text unchanged?", map.getNotePlainText, note_new
  expect "After reselect, is note plaintext?", map.hasRichTextNote, false

  # Plaintext notes copy & paste
  map.copy
  map.paste
  map.selectLastChildBranch
  s=map.getSelectionString
  expect "After copy& paste: New note unchanged?", map.getNotePlainText, note_plain
  expect "After copy& paste: New note Still plaintext?", map.hasRichTextNote, false
  map.remove

  # Plaintext notes undo & redo
  map.select @main_a
  map.setNotePlainText('Foobar')
  map.undo
  expect "Undo after setNotePlainText restores previous note", map.getNotePlainText, note_plain
  map.redo
  map.select @main_a
  expect "Redo restores previous note", map.getNotePlainText, 'Foobar'

  # Plaintext notes load & save
  note_org = IO.read('notes/note-plain.txt')
  map.loadNote("test/notes/note-plain.txt")
  expect "Load plain text note from file. Still plaintext?", map.hasRichTextNote, false
  expect "Note contains 'not bold'", map.getNotePlainText.include?("not bold"), true
  filepath = "#{@testDir}/save-note.txt"
  map.saveNote(filepath)
  expect "Save note to file. Check if it contains 'textMode=\"plainText\"'", IO.read(filepath).include?("textMode=\"plainText\""), true
  expect "Save note to file. Check if it contains 'not bold'", IO.read(filepath).include?("not bold"), true
  expect "Save note to file. Check new format: no longer contains '<b>' element", IO.read(filepath).include?("<b>"), false
  expect "Save note to file. Check new format: no longer contains '<![CDATA['", IO.read(filepath).include?("<![CDATA["), false
  expect "Save note to file. Check new format: contains 'text=\"Plaintext'", IO.read(filepath).include?("text=\"Plaintext"), true

  # Delete note
  map.setNotePlainText("")
  expect "setNotePlainText(\"\") deletes note", map.hasNote, false

  close_current_map

  # RichText basic actions
  map = init_map @testMapDefault
  map.select @main_a
  rt_note = '<vymnote  textMode="richText"><![CDATA[<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"> <html><head><meta name="qrichtext" content="1" /><style type="text/css"> p, li { white-space: pre-wrap; } </style></head><body style=" font-family:"Arial"; font-size:12pt; font-weight:400; font-style:normal;"> <p style=" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:"DejaVu Sans Mono"; color:#000000;">Rich Text note with <b>not bold text</b></span></p></body></html>]]></vymnote>'
  map.parseVymText(rt_note)
  expect "parseVymText of richText note produces note", map.hasNote, true
  expect "parseVymText of richText note produces richText note", map.hasRichTextNote, true
  map.select @center_0
  map.select @main_a
  expect "After reselect, is note RichText?", map.hasRichTextNote, true


  # RichText notes copy & paste
  rt_note = map.getNoteXML
  map.copy
  map.paste
  map.selectLastChildBranch
  s = map.getSelectionString
  expect "After copy & paste: New note Still RichText?", map.hasRichTextNote, true
  expect "After copy & paste: New note unchanged?", map.getNoteXML, rt_note
  map.remove

  # RichText notes undo & redo
  map.select @main_a
  map.setNotePlainText('Foobar')
  map.undo
  expect "Undo after setNotePlainText restores RichText note", map.getNoteXML, rt_note
  map.redo
  map.select @main_a
  expect "Redo restores previous plaintext note", map.getNotePlainText, 'Foobar'

  # RichText notes load & save
  map.loadNote("test/notes/note.html")
  expect "Load HTML note from file and try to detect textMode. Is RichText?", map.hasRichTextNote, true
  filepath = "#{@testDir}/save-note.txt"
  map.saveNote(filepath)
  expect "Save note to file. Check if it contains 'textMode=\"richText\"'", IO.read(filepath).include?("textMode=\"richText\""), true
  expect "Save note to file. Check if it contains 'bold'", IO.read(filepath).include?("bold"), true
  expect "Save note to file. Check new format: no longer contains '<b>' element", IO.read(filepath).include?("<b>"), false
  expect "Save note to file. Check new format: no longer contains '<![CDATA['", IO.read(filepath).include?("<![CDATA["), false
  expect "Save note to file. Check new format: contains 'text=\"&lt;'", IO.read(filepath).include?("text=\"&lt;"), true

  # Delete note
  map.setNotePlainText("")
  expect "setNotePlainText(\"\") deletes note", map.hasNote, false

  # Compatibility with version < 2.5.0  # FIXME-2 missing

  close_current_map
end

#######################
def test_references
  heading "References"
  map = init_map @testMapDefault

  map.select @main_a
  url = "www.insilmaril.de"
  map.setURL url
  expect "setURL to '#{url}'", map.getURL, url

  map.undo
  expect "undo setURL", map.getURL, ""
  map.redo
  expect "redo setURL", map.getURL, url
  map.setURL ""
  expect "setURL: unset URL with empty string", map.getURL, ""

  vl = "default.vym"
  map.setVymLink vl
  s = map.getVymLink
  expect "setVymLink returns absolute path", map.getFileDir + vl, s
  map.undo
  expect "undo: setVymLink", map.getVymLink, ""
  map.redo
  expect "redo: setVymLink", map.getVymLink, s
  map.undo

  close_current_map
end

#######################
def test_standard_flags
  heading "Standard flags"
  map = init_map @testMapDefault
  map.select @main_a

  def set_flags (map, flags)
    flags.each do |f|
      map.setFlagByName( f )
      expect "Flag set: #{f}", map.hasActiveFlag( f ), true
    end
  end

  def unset_flags (map, flags)
    flags.each do |f|
      map.unsetFlagByName( f )
      expect "Flag unset: #{f}", map.hasActiveFlag( f ), false
    end
  end

  # Group standard-mark
  set_flags( map, [ "exclamationmark","questionmark"] )

  # Group standard-status
  set_flags( map, [ "hook-green",
    "wip",
    "cross-red",
    "stopsign" ] )

  # Group standard-smiley
  smileys = [ "smiley-good",
      "smiley-sad",
      "smiley-omb" ]
  set_flags( map, smileys )

  # Group standard-arrow
  set_flags( map, [ "arrow-up",
    "arrow-down",
    "2arrow-up",
    "2arrow-down" ] )

  # Group standard-thumb
  set_flags( map, [ "thumb-up", "thumb-down" ] )

  # Without group
  set_flags( map, [ "clock",
    "phone",
    "lamp",
    "rose",
    "heart",
    "present",
    "flash",
    "info",
    "lifebelt" ] )

  unset_flags( map, smileys )

  map.clearFlags

  expect "clearFlags cleared exclamationmark", map.hasActiveFlag( "exclamationmark" ), false
  expect "clearFlags cleared smiley-good", map.hasActiveFlag( "smiley-good" ), false


  # Toggling flags
  a = ["stopsign", "lifebelt"]
  a.each do |flag|
    #puts "Flag is now: #{flag}"
    map.toggleFlagByName flag
    expect "toggleFlag: flag #{flag} activated", map.hasActiveFlag(flag), true

    map.toggleFlagByName flag
    expect "toggleFlag: flag #{flag} deactivated", map.hasActiveFlag(flag), false
  end

  close_current_map
end

#######################
def test_user_flags
  heading "User flags"
  map = init_map "maps/test-userflag.vym"

  map.select @branch_a
  flagName = "userflag-vym"
  expect "Has active flag '#{flagName}'", map.hasActiveFlag(flagName), true

  # FIXME-2 cont here
  close_current_map
end

#######################
def test_xlinks
  heading "XLinks:"
  map = init_map @testMapDefault

  map.addXLink("mc:0,bo:0","mc:0,bo:1",2,"#ff0000","Qt::DashDotLine")
  map.selectLatestAdded
  expect "Default color of XLink", map.getXLinkColor, "#ff0000"
  expect "Default width of XLink", map.getXLinkWidth.to_i, 2
  expect "Default style of XLink", map.getXLinkPenStyle, "Qt::DashDotLine"
  expect "Default style of XLink begin", map.getXLinkStyleBegin, "HeadFull"
  expect "Default style of XLink end",   map.getXLinkStyleEnd, "HeadFull"

  map.setXLinkWidth(3)
  expect "New width of XLink", map.getXLinkWidth.to_i, 3
  map.undo
  expect "Undo width of XLink", map.getXLinkWidth.to_i, 2

  map.setXLinkColor("#00ff00")
  expect "New color of XLink", map.getXLinkColor, "#00ff00"
  map.undo
  expect "Undo color of XLink", map.getXLinkColor, "#ff0000"

  map.setXLinkStyle("Qt::SolidLine")
  expect "New style of XLink", map.getXLinkPenStyle, "Qt::SolidLine"
  map.undo
  expect "Undo style of XLink", map.getXLinkPenStyle, "Qt::DashDotLine"

  map.setXLinkStyleBegin("None")
  expect "New style of XLink begin", map.getXLinkStyleBegin, "None"
  map.undo
  expect "Undo style of XLink begin", map.getXLinkStyleBegin, "HeadFull"

  map.setXLinkStyleEnd("None")
  expect "New style of XLink end", map.getXLinkStyleEnd, "None"
  map.undo
  expect "Undo style of XLink end", map.getXLinkStyleEnd, "HeadFull"

  close_current_map
end

#######################
def test_tasks
  heading "Tasks:"
  map = init_map "maps/test-tasks.xml"

  map.select @branch_a
  expect "After loading #{@branch_a} has no task", map.hasTask, false

  map.select @branch_b
  expect "After loading #{@branch_b} has task", map.hasTask, true
  expect "After loading #{@branch_b} task sleeps more than 1000 days",
    map.getTaskSleepDays.to_i > 1000, true

  map.select @branch_a
  map.toggleTask
  expect "Toggle task", map.hasTask, true

  date_today = DateTime.now
  delta_days = 123
  date_later = date_today + delta_days
  date_later_iso = date_later.strftime("%Y-%m-%dT%H:%M:%S")

  # Input: number of days
  date_new = delta_days
  expect "Set task sleep to number of days '#{date_new}' accepts input", map.setTaskSleep(date_new),  true
  expect "Set task sleep to number of days '#{date_new}' has correct sleep value '#{delta_days}' days", map.getTaskSleepDays.to_i, delta_days

  # Input: number of seconds
  date_new = "10s"
  expect "Set task sleep to number of seconds '#{date_new}' accepts input", map.setTaskSleep(date_new),  true

  # Input: number of hours
  date_new = "10h"
  expect "Set task sleep to number of hours '#{date_new}' accepts input", map.setTaskSleep(date_new),  true

  # Input: Date
  date_new = date_later.strftime("%Y-%m-%d")
  expect "Set task sleep to ISO Date '#{date_new}' accepts input", map.setTaskSleep(date_new), true
  expect "Set task sleep to ISO Date '#{date_new}' has correct sleep value '#{delta_days}' days", map.getTaskSleepDays.to_i, delta_days

  date_new = date_later.strftime("%d.%m.")
  expect "Set task sleep to German short form '#{date_new}' accepts input '#{date_new}'", map.setTaskSleep(date_new), true
  expect "Set task sleep to German short form '#{date_new}' has correct sleep value (days)", map.getTaskSleepDays.to_i, delta_days

  date_new = date_later.strftime("%d.%m.%Y")
  expect "Set task sleep to German long form '#{date_new}' accepts input '#{date_new}'", map.setTaskSleep(date_new), true
  expect "Set task sleep to German long form '#{date_new}' has correct sleep value (days)", map.getTaskSleepDays.to_i, delta_days

  # Input: Invalid strings
  date_new = "invalidDate"
  expect "Set task sleep to '#{date_new}' should fail", map.setTaskSleep(date_new), false

  date_new = date_later.strftime("%d %m.%Y")
  expect "Set task sleep to '#{date_new}' should fail", map.setTaskSleep(date_new), false

  # DateTime
  date_new = date_later_iso
  expect "Set task sleep to ISO DateTime '#{date_new}' accepts input", map.setTaskSleep(date_new), true
  expect "Set task sleep to ISO DateTime '#{date_new}' returns correct sleep value '#{date_later_iso}'", map.getTaskSleep, date_later_iso

  # Time only
  date_later = date_today

  date_new = "12:34"
  date_later_iso = date_today.strftime("%Y-%m-%dT12:34:00")
  expect "Set task sleep to time '#{date_new}' accepts input", map.setTaskSleep(date_new), true
  expect "Set task sleep to time '#{date_new}' returns correct sleep value '#{date_later_iso}'",
    map.getTaskSleep, date_later_iso

  date_new = "2:4"
  date_later_iso = date_today.strftime("%Y-%m-%dT02:04:00")
  expect "Set task sleep to time '#{date_new}' accepts input", map.setTaskSleep(date_new), true
  expect "Set task sleep to time '#{date_new}' returns correct sleep value '#{date_later_iso}'",
    map.getTaskSleep, date_later_iso

  date_new = "03:05"
  date_later_iso = date_today.strftime("%Y-%m-%dT03:05:00")
  expect "Set task sleep to time '#{date_new}' accepts input", map.setTaskSleep(date_new), true
  expect "Set task sleep to time '#{date_new}' returns correct sleep value '#{date_later_iso}'",
    map.getTaskSleep, date_later_iso

  close_current_map
end

######################
def test_saving
  heading "Saving:"
  map = init_map @testMapDefault
  #
  # Save selection without overwriting original map
  map.select @branch_a
  fn = @testDir + "/test-saveSelection.vyp"
  map.saveSelection(fn)
  expect "#Save selection: #{@branch_a} to #{fn}", File.file?(fn), true

  close_current_map

  map = init_map fn
  map.select @center_0
  expect "Save selection: After loading of #{fn} #{@center_0} is ok", map.getHeadingPlainText, "branch a"
  map.select @main_a
  expect "Save selection: After loading of #{fn} #{@main_a} is ok", map.getHeadingPlainText, "branch a1"

  close_current_map
end

######################
def test_load_legacy_maps
  heading "Load legacy maps:"

  map = init_map "maps/legacy/legacy-text-2.4.0.xml"
  map.select @branch_a
  expect "Heading with plaintext as characters is read", map.getHeadingPlainText, "Heading in characters"

  map.select @center_0
  expect "Checking parsing 'absPos': x-position of #{@center_0} is ok", map.getPosX().to_f, 314
  expect "Checking parsing 'absPos': y-position of #{@center_0} is ok", map.getPosY().to_f, 0
  map.select @main_a
  expect "Checking parsing 'relPos': x-position of #{@main_a} is ok", map.getPosX().to_f, 123
  expect "Checking parsing 'relPos': y-position of #{@main_a} is ok", map.getPosY().to_f, 42

  close_current_map

  map = init_map "maps/legacy/time-management-1.13.33.vym"
  map.select @main_a
  s = "To see an explanation"
  expect "<heading> using characters: Heading includes '#{s}'", map.getHeadingPlainText.include?(s), true
  expect "<vymnote> using <html>: creates RichText note", map.hasRichTextNote, true
  s = "time management"
  expect "<vymnote> using <html>: Note contains '#{s}'", map.getNotePlainText.include?(s), true

  close_current_map

  map = init_map "maps/legacy/lifeforms-2.1.11.vym"
  map.select @center_0

  s = "Life forms"
  expect "<heading> using characters and HTML: includes '#{s}'", map.getHeadingXML.include?(s), true
  s = "html"
  expect "<heading> using characters and HTML: includes '#{s}'", map.getHeadingXML.include?(s), true

  close_current_map
end

#######################
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


begin
  options = {}
  OptionParser.new do |opts|
    opts.banner = "Usage: vym-test.rb [options]"

    opts.on('-d', '--directory  NAME', 'Directory name') { |s| options[:testDir] = s }
  end.parse!

  @testDir = options[:testDir]
  @testMapDefault= "maps/test-default.vym"
  @testMapFrames = "maps/test-frames.vym"

  $tests_passed    = 0
  $tests_failed    = 0
  $tests_warnings  = 0
  $tests_total     = 0

  #######################
  @center_0="mc:0"
  @main_a="mc:0,bo:0"
    @branch_a=@main_a+",bo:0"
    @branch_b=@main_a+",bo:1"
    @branch_c=@main_a+",bo:2"
  @main_b="mc:0,bo:1"
  @center_1="mc:1"
  @center_2="mc:2"
  @center_3="mc:3"

  @n_centers = 2

  instance_name = 'test'

  vym_mgr = VymManager.new
  #vym_mgr.show_running

  @vym = vym_mgr.find(instance_name)

  if !@vym
    puts "Couldn't find instance name \"#{instance_name}\", please start one:"
    puts "vym -l -n \"#{instance-name}\" -t test/default.vym"
    exit
  end

  test_vym
  test_basics

  #test_adding_branches
  #test_adding_maps
  #test_attributes
  #test_bugfixes
  #test_copy_paste
  #test_delete_parts
  #test_export
  #test_extrainfo
  #test_frames
  ##test_headings  # FIXME-2 no tests available
  #test_history
  test_load_legacy_maps # FIXME-2 fails currently
  #test_modify_branches
  #test_moving_parts
  #test_notes
  #test_references
  #test_saving
  #test_scrolling
  #test_slides
  #test_standard_flags
  #test_tasks
  #test_user_flags
  #test_xlinks

  summary

end

