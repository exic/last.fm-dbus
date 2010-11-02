#!/usr/bin/perl
# run from ..

print "RUN SVN-CLEAN FIRST OR YOU RISK GETTING DUD STRINGS!\n";

opendir( DIR, "i18n" );
@files = grep( /lastfm(.*)\.ts$/, readdir( DIR ) );
closedir( DIR );

foreach $file (@files) {
   system( "lupdate src -ts i18n/${file}" );
}


print "Update complete, now you should prolly commit.\n";
