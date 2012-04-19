#!/usr/bin/env ruby

require "#{ENV['PWD']}/scripts/vym-ruby"

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
    puts "Failed: #{comment}. Expected #{v_exp}, but got #{v_real}"
    $tests_failed += 1
    waitkey
  end  
    $tests_total += 1
end

def heading (s)
  puts "\n#{s}\n#{'-' * s.length}\n"
end

def init_map
  # FIXME Missing: check or init default map 
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
center_0="mc:0"
main_a="mc:0,bo:0"
  branch_a=main_a+",bo:0"
  branch_b=main_a+",bo:1"
  branch_c=main_a+",bo:2"
main_b="mc:0,bo:1"


#######################
heading "Basic checks:"
init_map
vym.select main_a
expect "select", main_a, vym.getSelectString
expect "getHeading", "Main A", vym.getHeading
expect "branchCount", 3, vym.branchCount

vym.selectLastBranch
expect "selectLastBranch", "branch c", vym.getHeading

#######################
heading "Adding branches:"
init_map
vym.select main_a
n=vym.branchCount
vym.addBranch()
expect( "addBranch", n+1, vym.branchCount )

vym.selectLatestAdded
expect "selectLatestAdded", vym.getSelectString, main_a+",bo:3"

vym.undo
expect( "Undo: addBranch", n, vym.branchCount )

init_map
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

init_map
vym.select branch_a
vym.addBranchBefore
vym.select main_a
expect( "addBranchBefore: check branchcount", n, vym.branchCount)
vym.select branch_a
expect( "addBranchBefore: check heading", "", vym.getHeading)
vym.undo
vym.select main_a
expect( "Undo: addBranchBefore", n, vym.branchCount )

#######################
heading "Adding maps"
init_map
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
expect( "Undo: check if #{branch_a} is back", 3, vym.branchCount )

init_map
vym.select main_a
vym.addMapInsert "test/default.vym",1
vym.select main_a
expect( "addMapInsert: branch count", n+1, vym.branchCount )
vym.select main_a + ",bo:1"
expect( "addMapInsert: new heading", vym.getHeading, "MapCenter 0")

vym.undo
vym.select main_a
expect( "Undo: check branch count in #{main_a}", 3, vym.branchCount )
vym.select branch_b
expect( "Undo: check heading of  #{branch_b}", "branch b", vym.getHeading)

#######################
heading "Scrolling and unscrolling"
init_map
vym.select main_a
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
vym.select branch_a
vym.scroll
vym.select main_a
vym.unscrollChildren
vym.select branch_a
expect "unscrollChildren", vym.isScrolled, false
vym.undo
expect "undo unscrollChildren", vym.isScrolled, true
vym.unscroll
vym.select branch_a
vym.unscroll


#######################
heading "Moving parts"
init_map
vym.select branch_a
vym.moveDown
vym.select branch_a
expect "Moving down", vym.getHeading, "branch b"
vym.undo
vym.select branch_a
expect "Undo Moving down", vym.getHeading, "branch a"

init_map
vym.select branch_b
vym.moveUp
vym.select branch_a
expect "Moving up", vym.getHeading, "branch b"
vym.undo
vym.select branch_b
expect "Undo Moving up", vym.getHeading, "branch b"

init_map
vym.select main_b
n=vym.branchCount
vym.select branch_a
vym.relinkTo main_b,0,0,0
vym.select main_b
expect "RelinkTo #{main_b}: branchCount increased there", n+1, vym.branchCount
vym.undo
vym.select branch_b
expect "Undo: RelinkTo #{main_b}: branchCount decreased there", n, vym.branchCount

init_map
vym.select branch_a
n=vym.branchCount
vym.select main_b
vym.relinkTo branch_a, 1, 0, 0
vym.select branch_a
expect "RelinkTo #{branch_a}, pos 1: branchCount increased there", n+1, vym.branchCount
vym.select "#{branch_a},bo:1"
expect "RelinkTo #{branch_a}, pos 1: Mainbranch really moved", "Main B", vym.getHeading
vym.undo
vym.select center_0
expect "Undo RelinkTo pos 1: branchCount of center", 2, vym.branchCount
# FIXME still has wrong position, check position

#######################
heading "Modifying branches"
init_map
vym.select branch_a
vym.setHeading "Changed!"
expect "setHeading","Changed!",vym.getHeading
vym.undo
expect "Undo: setHeading","branch a",vym.getHeading
vym.redo
expect "redo: setHeading", vym.getHeading, "Changed!" 
vym.undo

#######################
heading "Flags"
init_map
vym.select main_a

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
expect "clearFlags cleared exclamationmark", 
  vym.hasActiveFlag( "exclamationmark" ), 
  false
expect "clearFlags cleared smiley-good", 
  vym.hasActiveFlag( "smiley-good" ), 
  false

vym.toggleFlag "lifebelt"
expect "toggleFlag: flag activated", vym.hasActiveFlag("lifebelt"),true
vym.toggleFlag "lifebelt"
expect "toggleFlag: flag deactivated", vym.hasActiveFlag("lifebelt"),false

#######################
heading "Deleting parts"
init_map
vym.select main_a
n=vym.branchCount
vym.select branch_a
m=vym.branchCount
vym.delete
vym.select main_a
expect "Delete branch: branchcount", n-1, vym.branchCount
vym.undo
vym.select main_a
expect "Undo Delete branch: branchcount parent", n, vym.branchCount
vym.select branch_a
expect "Undo Delete branch: branchcount restored branch", m, vym.branchCount

init_map
vym.select branch_a
n=vym.branchCount
vym.deleteChildren
vym.select branch_a
expect "deleteChildren: branchcount", 0, vym.branchCount
vym.undo
vym.select branch_a
expect "Undo: deleteChildren: branchcount", n, vym.branchCount

init_map
vym.select main_a
n=vym.branchCount
vym.select branch_a
m=vym.branchCount
vym.deleteKeepChildren
vym.select main_a
expect "deleteKeepChildren: branchcount", n+m-1,vym.branchCount
vym.undo
vym.select main_a
expect "Undo: deleteKeepChildren: branchcount of parent", n,vym.branchCount
vym.select branch_a
expect "Undo: deleteKeepChildren: branchcount of branch", m,vym.branchCount

#######################
heading "Copy, cut & Paste"
init_map
vym.select main_a
n=vym.branchCount

vym.copy
vym.paste
vym.selectLastBranch
s=vym.getSelectString
expect "Normal paste of branch, check heading of #{s}", vym.getHeading, "Main A"

vym.undo
vym.select main_a
expect "Undo paste: branchCount of #{main_a}", vym.branchCount, n

vym.redo
vym.select s
expect "redo paste: check heading", vym.getHeading, "Main A"

vym.cut
vym.select main_a
expect "cut: branchCount of #{main_a}", vym.branchCount, n
vym.paste
vym.selectLastBranch
s=vym.getSelectString
expect "Normal paste of branch, check heading of #{s}", vym.getHeading, "Main A"
vym.cut 

#######################
heading "History"
init_map

#######################
summary

=begin
# Untested commands:
#
addSlide
addXlink
colorBranch
colorSubtree
cycleTask
delete (image)
deleteSlide
exportAO
exportASCII
exportHTML
exportImage
exportLaTeX
exportPDF
exportPDF
exportSVG
exportXML
importDir
loadImage
loadNote
moveSlideDown
moveSlideUp
move
moveRel
note2URLs
redo  
  so far:
    paste
    setHeading
relinkTo (for images)
saveImage
saveNote
selectLastImage
selectLatestAdd
setTaskSleep
setFrameIncludeChildren
setFrameType
setFramePenColor
setFrameBrushColor
setFramePadding
setFrameBorderWidth
setHideExport
setIncludeImagesHorizontally
setIncludeImagesVertically
setHideLinksUnselected
setMapAuthor
setMapComment
setMapBackgroundColor
setMapDefLinkColor
setMapLinkStyle
setNote
setScale
setSelectionColor
setURL
setVymLink
sortChildren
toggleFrameIncludeChildren
toggleTarget
toggleTask
=end
