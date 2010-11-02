#!/usr/bin/perl
#
# brief: This script builds a release build from scratch and packages it
#        into an installer. You need %QTDIR% and %VSDIR% set in your environment
#
#        Must be run from trunk.
#
# pre-requisites: perl, svn, tar, bzip2, 
#                 Inno Setup 5 (install to default location or set %ISDIR%)
#

use File::Path;
use File::Copy;
use File::Find;


########################################################################
if ($ARGV[0] eq "--upload")
{
    my $VERSION = getVersion();
    my $r = system( "perl",
                    "dist/breakpad-upload-symbolstore.pl", 
                    "dist/Last.fm-$VERSION-win.symbols.tar.bz2" );
    exit $r;
}
########################################################################

my $VERSION = getVersion();

my $ISSFILE = "dist/build-installer-win";
if ($ARGV[0] eq "--jp")
{
    $ISSFILE = "$ISSFILE-jp";
}
$ISSFILE = "$ISSFILE.iss";

my $QTDIR = quotemeta( $ENV{'QTDIR'} or die $! );
my $VSDIR = quotemeta( $ENV{'VSDIR'} or die $! );
$QTDIR =~ s/\\/\\\\/g; #double escape \s for the shell :(
$VSDIR =~ s/\\/\\\\/g;

########################################################################
header( "Building release and installer for Last.fm-$VERSION" );

########################################################################
print( "\nDon't forget the change log! ([Enter] to continue, [Ctrl-C] to abort)\n" );
$dummy = <STDIN>;

header( "Cleaning" );
    run( "perl", "tools/svn-clean.pl" ) or die $! unless $ARGV[0] eq "--no-clean";
    mkpath( "build/syms" );

header( "Substituting strings in various files" );
    sub findVersionFiles()
    {
        if ($_ =~ /\.rc$/i || $_ =~ /\.manifest$/i || $_ =~ /\.iss$/i)
        {
            push( @versionFiles, $File::Find::name );
        }
    }
    find( \&findVersionFiles, "src" );
    find( \&findVersionFiles, "dist" );

    updateVersion( @versionFiles );

    system( 'perl -pi".bak" -e "s/%QTDIR%/' . $QTDIR . '/g" ' . $ISSFILE );
    system( 'perl -pi".bak" -e "s/%VSDIR%/' . $VSDIR . '/g" ' . $ISSFILE );

header( "qmake" );
    run( "qmake", '"CONFIG += breakpad release"' ) or die $!;

header( "nmake" );
    run( "nmake release" ) or die $!;

header( "Translations" );
    if ($ARGV[0] eq "--jp")
    {
        mkdir ( "bin/data/i18n" );
        system( "lrelease i18n/lastfm_jp.ts -qm bin/data/i18n/lastfm_jp.qm" );
        system( "lrelease i18n/qt_jp.ts -qm bin/data/i18n/qt_jp.qm" );
    }
    else
    {
        run( "perl", "dist\\i18n.pl" ) or die $!;
    }

header( "Installer" );
    #my $ISDIR = $ENV{'ISDIR'} or "c:\\Program Files\\Inno Setup 5";
    #$ISDIR =~ s/\\/\//g;
    #run( "$ISDIR\\iscc.exe", "$ISSFILE" ) or die $!;

    if ($ARGV[0] eq "--jp")
    {
        run( "c:/Program Files/Inno Setup 5/iscc.exe", "dist/build-installer-win-jp.iss" ) or die $!;
    }
    else
    {
        run( "c:/Program Files/Inno Setup 5/iscc.exe", "dist/build-installer-win.iss" ) or die $!;
    }



header( "Reverting pre-processed version files" );
    # We're doing this so that the version files will go back to their initial state
    # of 0.0.0.0, thus enabling further rebuilds without having to do a clean
    # checkout. A bit hacky, but pragmatic methinks.
    svnRevert( @versionFiles );

header( "Building symbolstore" );
    dumpSyms( "bin" );
    dumpSyms( "$ENV{QTDIR}/lib" );
    chdir( "build/syms" );
    run( "tar", "cjf", "../../dist/Last.fm-$VERSION-win.symbols.tar.bz2", "." );

header( "done!" );
    print "To upload the symbols, issue the following command:\n";
    print "       perl dist\\build-release-win.pl --upload";



########################################################################
sub header
{
    print "\n==> $_[0]\n";
}

sub getVersion
{
    my $revision = getSVNRevision();

    # Due to a MS quirk, no part of the version number can be bigger than 16 bits, so we mod it
    $revision = $revision % 50000;
    
    open DATA, 'src/version.h' or die;
    my @lines = <DATA>;
    close( DATA );

    foreach my $line (@lines)
    {
        if ( $line =~ m/LASTFM_CLIENT_VERSION "(\d+\.\d+\.\d+)(\S*)"/ )
        {
            my $oldver = $1 . $2;
            my $newver = $1 . "." . $revision;

            if ( !( $oldver eq $newver) )
            {
                system( "perl -pi\".bak\" -e \"s/$oldver/$newver/g\" src/version.h" );
            }
            return $newver;
        }
    }

    die;
}

sub getSVNRevision
{
    @output = `svn info`;

    foreach my $line (@output)
    {
      if ( $line =~ m/Last Changed Rev: (\d+)/ )
      {
          return $1;
      }
    }

    die;
}

sub updateVersion
{
    my @list = @_;
    foreach $file ( @list )
    {
        system( "perl -pi\".bak\" -e \"s/0\\.0\\.0\\.0/$VERSION/g\" $file" );
        my $versionCommas = $VERSION;
        $versionCommas =~ s/\./,/g;
        system( "perl -pi\".bak\" -e \"s/0,0,0,0/$versionCommas/g\" $file" );
        print $file . "\n";
    }
}

sub svnRevert
{
    my @list = @_;
    foreach $file ( @list )
    {
        system( "svn revert $file" );
    }
}

sub dumpSyms
{
    ($d = $_[0]) =~ s/\\/\//g; #swap \ with / as \s break the following :(

    opendir( DIR, $d );
    foreach $pdb (grep( /\.pdb$/, readdir( DIR ) ))
    {
        if ( !grep( /d4\.pdb/, $pdb ) )
        {
            system( "perl", "dist\\breakpad-make-symbolstore.pl", "-c", "dist/win/dump_syms.exe", "build/syms", "$d/$pdb" );
        }
    }
    closedir( DIR );
}

sub run
{
    return system( @_ ) == 0;
}
########################################################################
