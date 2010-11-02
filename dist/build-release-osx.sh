#!/bin/bash
#
# Usage: dist/build-relese-osx.sh [-j] [--no-clean]
#
# Adding the -j parameter results in building a japanese version.
################################################################################


function header {
    echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

function die {
    exit_code=$?
    echo $1
    exit $exit_code
}
################################################################################


OLD_VERSION=$(cat src/version.h | grep LASTFM_CLIENT_VERSION | cut -d\" -f2)
svnRevision=`svn info | grep "Last Changed Rev" | cut -d' ' -f4`
let modRev=`echo $svnRevision`%50000
VERSION=$(echo $OLD_VERSION | cut -d'.' -f1,2,3).$modRev
ROOT=`pwd`

QTDIR=`which qmake`
QTDIR=`dirname $QTDIR`
QTDIR=`dirname $QTDIR`
test -L "$QTDIR" && QTDIR=`readlink $QTDIR`

export QMAKESPEC='macx-g++'
export QTDIR
export VERSION
################################################################################


CLEAN='1'
BUILD='1'
NOTQUICK='1'
CREATEDMG='1'

while [ $# -gt 0 ]
do
    case "$1" in
        --uninstall)
            # we must kill as existing processes will write their config on exit
            killall -9 Last.fm iPodScrobbler
            rm -r /Applications/Last.fm.app
            cd ~/Library
            rm -r iTunes/iTunes\ Plug-ins/AudioScrobbler.bundle
            rm -r Application\ Support/Last.fm
            rm Preferences/fm.last.Last.fm.plist
            exit 0;;
        --upload)
            curl -T dist/Last.fm-$VERSION.symbols.tar.bz2 http://oops.last.fm/symbols/upload
            exit 0;;
        --revert)
            svn revert src/version.h
            svn revert --recursive res
            exit 0;;
        --no-clean)
            header "Skipping clean."
            unset CLEAN;;
        --no-build)
            header "Skipping build."
            unset BUILD;;
        -j | --jp)
            header "Building version $VERSION in Japanese."
            export JAPANESE='1';;
        --quick)
            header "Skipping some non essential steps."
            unset NOTQUICK;;
        --no-dmg)
            header "Skipping DMG creation."
            unset CREATEDMG;;
        --install)
            header "Installing bundle to /Applications."
            export INSTALL='1';;
        --mxcl)
            header "mxcl mode: --no-dmg --quick --install"
            export INSTALL='1'
            unset CREATEDMG
            unset NOTQUICK;;
        *)
            echo "Unknown argument \`$1\'."
            exit 1;;
    esac
    shift
done


header "Preparing Last.fm-$VERSION.dmg"


set -e

if [ $CLEAN ]
then
    header "Cleaning source directory..."
    tools/dist-clean.sh

    header "Configuring project (qmake)..."
    DEFINES='DEFINES += NDEBUG'
    test $JAPANESE && DEFINES="$DEFINES HIDE_RADIO"
    test $NOTQUICK && CONFIG_OPTS="ppc breakpad"
    qmake "CONFIG += release app_bundle x86 warn_off $CONFIG_OPTS" "CONFIG -= debug warn_on" "$DEFINES" 
else
    # delete some stuff though as it isn't built and prevents errors later
    pushd bin/Last.fm.app/Contents &> /dev/null
    rm -rf MacOS/data
    rm -rf Frameworks
    rm -rf MacOS/sqldrivers
    rm -rf MacOS/PlugIns
    rm -f MacOS/LastFM
    popd &> /dev/null
fi

# fix b0rked fplib build process
mkdir -p build/fplib/release

if [[ $BUILD ]]
then
    [ $OLD_VERSION != $VERSION ] && sed -i.old -e "s/$OLD_VERSION/$VERSION/g" src/version.h

    header "Starting build..."
    nice -n 20 make -j2

    header "Building iTunes plugin..."
    svn co svn+ssh://svn.last.fm/svn/clientside/branches/plugins-1.5/iTunes build/iTunesPlugin
    export DEVELOPER_SDK_DIR=/Developer/SDKs
    xcodebuild -configuration Deployment -project build/iTunesPlugin/iTunes\ Scrobbler.xcodeproj
fi


set +e


header "Updating plist files..."
    cp dist/mac/Info.plist.in bin/Last.fm.app/Contents/Info.plist
    perl -pi -e 's/0\.0\.0\.0/'$VERSION'/g' bin/Last.fm.app/Contents/Info.plist
    perl -pi -e 's/0\.0\.0/'`echo $VERSION | cut -d'.' -f1,2,3`'/g' bin/Last.fm.app/Contents/Info.plist


if [[ $NOTQUICK ]]
then
    header "Generating qm files..."
    dist/i18n.pl
fi

header "Assembling application bundle..."
    pushd bin > /dev/null
    # copy the libs and executables we need
    # the exes link to *.x.dylib for some reason, so we have to copy those
    for x in `find . -maxdepth 1 -name \*.dylib | grep '^[^0-9]*\.[0-9]\.dylib'` \
             `find . -maxdepth 1 -name \*.dylib -prune -o -type f -print`
    do
        # the Mach-O bit is to stop us copying shell scripts and DLLs etc.
        tmp=$(file -L "$x" | grep 'Mach-O')
        test -n "$tmp" && cp "$x" Last.fm.app/Contents/MacOS
    done

    # copy misc files that we need
    cp -R services Last.fm.app/Contents/MacOS
    cp -R extensions Last.fm.app/Contents/MacOS
    cp -R data Last.fm.app/Contents/MacOS
    cp iPodScrobbler Last.fm.app/Contents/MacOS
    
    cp -R $ROOT/dist/mac/Resources Last.fm.app/Contents
    cp -R $ROOT/dist/mac/MacOS Last.fm.app/Contents
    cp $ROOT/COPYING Last.fm.app/Contents/
    cp $ROOT/ChangeLog.txt Last.fm.app/Contents/ChangeLog
        
    # cleanup
    find Last.fm.app -name .svn | xargs rm -rf

    cd Last.fm.app
    $ROOT/dist/mac/add-Qt-to-bundle.sh \
                   'QtCore QtGui QtXml QtNetwork QtSql' \
                   imageformats \
                   sqldrivers/libqsqlite.dylib

    header deposx
    $ROOT/dist/mac/deposx.sh
    
    #HACK avoid issues when upgrading due to renaming of executable for 1.3.0
    cd Contents/MacOS
    ln -s Last.fm LastFM

    # copy over iTunes plugin, don't deposx it
    mkdir PlugIns
    cp -R $ROOT/build/iTunesPlugin/build/Deployment/*.bundle PlugIns

    # symlink to iPodScrobbler to avoid dock icon showing up
    cd ../Resources
    ln -s ../MacOS/iPodScrobbler .

    popd > /dev/null


if [[ $NOTQUICK ]]
then
    header "Building dump_syms..."
    d=src/breakpad/external/src
    g++ -I$d -I$d/common/mac -o build/dump_syms \
        -framework Foundation -lcrypto \
        $d/tools/mac/dump_syms/dump_syms_tool.m \
        $d/common/mac/*.cc \
        $d/common/mac/dump_syms.mm


    header "Building breakpad symbolstore..."
    mkdir -p build/syms
    files=`find bin/Last.fm.app -type f -a -perm -100`
    # stolen from mozilla Makefile.in
    echo $files | xargs file -L \
                | grep "Mach-O" \
                | grep -v "for architecture" \
                | cut -f1 -d':' \
                | xargs dist/breakpad-make-symbolstore.pl \
    	                -a "ppc i386" \
    	                build/dump_syms \
    	                build/syms \
                > build/syms/Last.fm-$VERSION-mac.symbols.txt


    header "Stripping binaries..."
    for x in $files
    do
    	if [[ "`file $x | grep library`" != "" ]]
    	then
    	    echo "L $x"
    	    strip -S -x $x
    	else
    	    echo "F $x"
    	    strip $x
    	fi
    done

    header "Creating update.tar.bz2..."
    pushd bin/Last.fm.app/Contents &> /dev/null
    tar cjf ../../../dist/Last.fm_Mac_Update_$VERSION.tar.bz2 .
    popd &> /dev/null


    header "Packing symbols..."
    pushd build/syms &> /dev/null
    tar cjf ../../dist/Last.fm-$VERSION.symbols.tar.bz2 .
    popd &> /dev/null
fi # NOTQUICK


if [[ $CREATEDMG ]]
then
    header "Creating dmg..."
    dist/mac/create-dmg.sh bin/Last.fm.app
fi


if [[ $INSTALL ]]
then
    header "Installing bundle to /Applications..."
    echo "Uninstalling Last.fm and iTunes plugin first..."
    rm -rf ~/Library/iTunes/iTunes\ Plug-ins/AudioScrobbler.bundle
    killall Last.fm &> /dev/null
    echo "Installing new ones..."
    rm -rf /Applications/Last.fm.app
    cp -R bin/Last.fm.app /Applications
    cp -R bin/Last.fm.app/Contents/MacOS/PlugIns/AudioScrobbler.bundle ~/Library/iTunes/iTunes\ Plug-ins
fi


header Done!
    if [[ $CREATEDMG ]]
    then
        echo "I HOPE YOU UPDATED THE PLUGIN VERSION IF YOU NEEDED TO!!!"
        echo ""
        echo "The release products are in dist/"
    fi
    if [[ $NOTQUICK ]]
    then
        echo "To upload the symbols to oops.last.fm, issue the following command:"
        echo "       $0 --upload"
        echo
    fi
