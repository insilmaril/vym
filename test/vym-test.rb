#!/usr/bin/env ruby

require "#{ENV['PWD']}/scripts/vym-ruby"

$tests_passed = 0
$tests_failed = 0
$tests_total  = 0

def waitkey
  puts "Press return to continue..."
  STDIN.gets
end

def expect (comment, v_exp, v_real)
  if v_exp == v_real
    puts "    Ok: #{comment}"
    $tests_passed += 1
  else  
    puts "Failed: #{comment}. Expected #{v_exp}, but got #{v_real}"
    $tests_failed += 1
  end  
    $tests_total += 1
end

def summary
  puts "\nTests done  : #{$tests_total}"
  puts "Tests passed: #{$tests_passed}"
  puts "Tests failed: #{$tests_failed}"
end

vym_mgr=VymManager.new
#vym_mgr.show_running

vym=Vym.new(vym_mgr.find('test') )

#######################
main_a="mc:0,bo:0"
branch_a=main_a+",bo:0"
branch_b=main_a+",bo:1"


#######################
puts "\nBasic checks:"
vym.select main_a
expect "select", main_a, vym.getSelectString
expect "getHeading", "Main A", vym.getHeading
expect "branchCount", 3, vym.branchCount


#######################
puts "\nAdding branches:"
vym.select branch_a
n=vym.branchCount
vym.addBranch()
expect( "addBranch", n+1, vym.branchCount )

vym.undo
expect( "Undo: addBranch", n, vym.branchCount )

vym.select main_a
n=vym.branchCount
vym.select branch_a
vym.addBranch(-3)
vym.addBranch(-1)
vym.select main_a
expect( "addBranchAbove/Below", n+2, vym.branchCount )

vym.undo
vym.undo
expect( "Undo: addBranchAbove/Below", n, vym.branchCount )

vym.select branch_a
vym.addBranchBefore
vym.select main_a
expect( "addBranchBefore: check branchcount", n, vym.branchCount)
vym.select branch_a
expect( "addBranchBefore: check heading", "", vym.getHeading)
vym.undo
vym.select main_a
expect( "Undo: addBranchAbove/Below", n, vym.branchCount )

#######################
puts "\nAdding maps"
vym.select branch_a
vym.addMapReplace "test/default.vym"
vym.select main_a
expect( "addMapReplace: check branch count in #{main_a}", 3, vym.branchCount )
vym.select branch_a
expect( "addMapReplace: check if #{branch_a} is new", 2, vym.branchCount )

vym.undo
vym.select main_a
expect( "Undo: check branch count in #{main_a}", 3, vym.branchCount )
vym.select branch_a
expect( "Undo: check if #{branch_a} is back", 0, vym.branchCount )

vym.select main_a
vym.addMapInsert "test/default.vym",1
vym.select main_a
expect( "addMapInsert: branch count", n+1, vym.branchCount )
vym.select main_a + ",bo:1"
expect( "addMapInsert: new heading", vym.getHeading, "MapCenter")

vym.undo
vym.select main_a
expect( "Undo: check branch count in #{main_a}", 3, vym.branchCount )
vym.select branch_b
expect( "Undo: check heading of  #{branch_b}", "sub b", vym.getHeading)

summary
exit


#addSlide
#addXlink
#branchCount
#clearFlags
#colorBranch
#colorSubtree
#copy
#cut
#cycleTask
#delete
#deleteChildren
#deleteKeepChildren
#deleteSlide
#exportAO
#exportASCII
#exportHTML
#exportImage
#exportLaTeX
#exportPDF
#exportPDF
#exportSVG
#exportXML
#getHeading
#getSelectString
#importDir
#loadImage
#loadNote
#moveDown
#moveUp
#moveSlideDown
#moveSlideUp
#move
#moveRel
#nop
#note2URLs
#paste
#redo
#relinkTo
#saveImage
#saveNote
#scroll
#select
#selectLastBranch
#selectLastImage
#selectLatestAdded
#setFlag
#setTaskSleep
#setFrameIncludeChildren
#setFrameType
#setFramePenColor
#setFrameBrushColor
#setFramePadding
#setFrameBorderWidth
#setHeading
#setHideExport
#setIncludeImagesHorizontally
#setIncludeImagesVertically
#setHideLinksUnselected
#setMapAuthor
#setMapComment
#setMapBackgroundColor
#setMapDefLinkColor
#setMapLinkStyle
#setNote
#setScale
#setSelectionColor
#setURL
#setVymLink
#sortChildren
#toggleFlag
#toggleFrameIncludeChildren
#toggleTarget
#toggleTask
#undo
#unscroll
#unscrollChildren
#unsetFlag
#
