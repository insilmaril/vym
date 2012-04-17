#!/usr/bin/env ruby

require '/suse/uwedr/vym/code/scripts/vym-ruby'

vym_mgr=VymManager.new
#vym_mgr.show_running

vym=Vym.new(vym_mgr.find('test') )
#vym.show_methods
#puts "Modelcount: #{vym.modelCount}"

#vym.moveRel(50,50)
#vym.setHeading("foO","bar")
#puts "Returned: ",vym.getHeading

branch_a="mc:0,bo:0"
branch_b=branch_a+",bo:0"
puts "Select first branch of first mainbranch #{branch_b}"
vym.select branch_b
n=vym.branchCount

puts "Adding branches:"
vym.addBranch()
expect( "Branch count increased for adding a child", n+1, vym.branchCount )

vym.select branch_a
n=vym.branchCount
vym.select branch_b
vym.addBranch(-1)
vym.addBranch(1)
vym.select branch_a
expect( "Branch count increased for adding above and below", n+2, vym.branchCount )

