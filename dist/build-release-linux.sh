#!/bin/bash
# requirements: perl, bash, ubuntu, apt-get install libqt4-debug, devscripts, sharutils
# recommended: icecc
# authors: max@last.fm, chris@last.fm

function header {
	echo -e "\033[0;34m==>\033[0;0;1m $1 \033[0;0m"
}

oldversion=$(cat src/version.h | grep LASTFM_CLIENT_VERSION | cut -d\" -f2);
versionprefix=$(cat src/version.h | grep LASTFM_CLIENT_VERSION | cut -d\" -f2 | cut -d'.' -f1,2,3);
test -z $versionprefix && echo "Version is null :(" && exit 1
svnRevision=`svn info | grep "Last Changed Rev" | cut -d' ' -f4`
let modRev=`echo $svnRevision`%50000
VERSION=$versionprefix.$modRev
sed -i "s/$oldversion/$VERSION/g" src/version.h


if [[ $1 == "--upload" ]]
then
    dist/breakpad-upload-symbolstore.pl dist/Last.fm-$VERSION.symbols.tar.bz2
    scp dist/*.deb gimli:client/Linux
    scp dist/last.fm-$VERSION.src.tar.bz2 gimli:client/src
    exit $?
fi


if [[ $1 == "--auto-changelog" ]]
then
    date=`date -R`
    line="lastfm (1:$VERSION) stable;urgency=low\n  * Added more tasty moose\n\n -- Max Howell <max@last.fm>  $date\n"

    cd dist/debian
    mv changelog changelog.old
    echo -e "$line" | cat - changelog.old > changelog
    exit
fi




# forces use of the system Qt, not potential custom compilations
export PATH=/usr/lib/icecc/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/bin/X11:/usr/local/bin

QTDIR=`which qmake`
QTDIR=`dirname $QTDIR`
QTDIR=`dirname $QTDIR`
QTDIR=`readlink -f $QTDIR`


header "Cleaning"
tools/dist-clean.sh
mkdir -p build/fplib/release


TARBALL=last.fm-$VERSION.src.tar.bz2
header "Building $TARBALL"
cp -R . last.fm-$VERSION &> /dev/null
cd last.fm-$VERSION/bin
rm -rf imageformats
rm -rf Resources
rm -rf sqldrivers
rm *.dll
rm -f *.so*
cd ..
find -name \*.vcproj.\* | xargs rm
find -name \*.svn | xargs rm -rf
rm -rf tools
rm -rf debian
rm -rf build
rm -f *.sh *.bat
rm -rf LastFM.kdevelop LastFM.sln Audioscrobbler.sln.7.10.old Last.fm.xcodeproj
mv ChangeLog.txt ChangeLog
mv dist/tarball_files/* .
rm -rf dist
rm res/*.isl
rm -rf Microsoft.VC80.CRT
cd ..

tar cjf dist/$TARBALL last.fm-$VERSION
rm -rf last.fm-$VERSION


header "Building debian package"
qmake-qt4 -config release "CONFIG -= debug" "CONFIG += breakpad"
mv dist/debian .
debuild -i -us -uc -b || exit 1
patch -p0 -R < debian/patches/prefixification.diff
mv ../*.deb dist
mv debian dist

svn revert src/version.h

header "Building dump_syms"
d=src/breakpad/external/src
g++ -I$d -o build/dump_syms \
    $d/tools/linux/dump_syms/*.cc \
    $d/common/linux/dump_symbols.cc \
    $d/common/linux/file_id.cc \
    $d/common/md5.c


header "Building breakpad symbolstore"
mkdir build/syms
QtSyms=`dpkg -L libqt4-debug | grep \.so[.0-9]*\.debug$`
MySyms=`find bin -type f -a -perm -100`

# stolen from mozilla Makefile.in
echo "$MySyms $QtSyms" \
           | xargs file -L \
           | grep "ELF" \
           | cut -f1 -d':' \
           | xargs dist/breakpad-make-symbolstore.pl \
                   build/dump_syms \
                   build/syms \
           > build/syms/Last.fm-$VERSION-linux.symbols.txt


header "Packing symbols"
pushd build/syms
tar cjf ../../dist/Last.fm-$VERSION.symbols.tar.bz2 .
popd


header Done!
echo "Everything is in dist/"
echo "To upload the symbols, issue the following command:"
echo "       dist/build-release-linux.sh --upload"
echo
