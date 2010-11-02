#!/bin/ruby
# Maintained by Max Howell <max@last.fm>
# Feature requests to muesli <chris@last.fm>
# hehe ;-)

require 'find'

cpp_files = Array.new
excludes = [ '.', '..', '.svn', 'rtaudio', 'mpglib', 'zlib-1.2.3' ]

Find.find( File.expand_path( 'src' ) ) do |path|

    if (File.basename( path ) == "webservice.cpp")
        Find.prune
    end
    
    if (excludes.include?( File.basename( path ) ))
        Find.prune
    end

    if (File.extname( path ) == '.cpp')
        #print path, "\n"
        if (File.basename( path ).slice( 0, 3 ) != 'moc')
            cpp_files << path
        end
    end
end

cpp_files.each do |path|
    count = 0;
    
    File.new( path, "r" ).each_line do |line|
        skip = false
        count = count + 1

        ['dataPath', 'm_config', 'QT_TR_NOOP', 'Q_ASSERT', 'LOGL', 'LOG', 'qDebug', '#include', 'tr('].each do |token|
            if (line.include?( token ))
                skip = true
            end
        end

        if (skip == true)
            next
        end

        if (line.index( '"' ) != nil)
            puts "#{line.strip}:#{count}:#{path.sub( '/home/mxcl/src/trunk.debug/src/', '' )}"
        end
        :skip
    end
end
