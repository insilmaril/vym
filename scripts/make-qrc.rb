#!/usr/bin/env ruby
#
# Simple script to generate the resource file
# So far only png images are considered

require 'find'
require 'pathname'

outfile = 'vym.qrc'
subdirs = ['flags','icons']

content = "<!DOCTYPE RCC><RCC version=\"1.0\">\n"
content << "  <qresource>\n"
subdirs.each do |dir|
  Find.find(dir) do |path|
    # Create alias using the name of the subdir within dir
    abs_path  = Pathname.new( File.expand_path( path) )
    root_path = Pathname.new( File.expand_path( dir ) )
    rel_path  = abs_path.relative_path_from( root_path )
    
    if path =~ /.*\.png$/
      content << "    <file alias=\"#{rel_path.to_s}\">#{path}</file>\n" 
    end
  end
end
content << "  </qresource>"
content << "</RCC>"

File.write(outfile, content)