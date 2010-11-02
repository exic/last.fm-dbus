#!/bin/sh
# author: max@last.fm
# brief:  Produces a compressed DMG from a bundle directory
# usage:  Pass the bundle directory as the only parameter
# note:   This script depends on the Last.fm build system, and also requires
#         a template dmg to be placed in dist/mac/template.dmg
################################################################################


if [ -z $VERSION ]
then
    echo VERSION must be set
    exit 2
fi

if [ -z "$1" ]
then
    echo "Please pass the bundle.app directory as the first parameter."
    exit 3
fi
################################################################################


NAME=$(basename "$1" | perl -pe 's/(.*).app/\1/')
IN="$1"
TMP="build/dmg/$NAME"
OUT="dist/$NAME-$VERSION.dmg"
test $JAPANESE && LANG='jp' || LANG='en';
################################################################################


# clean up
rm -rf "$TMP"
rm -f "$OUT"

# create DMG contents and copy files
mkdir -p "$TMP/.background"
cp dist/mac/dmg_background.$LANG.png "$TMP/.background/background.png"
cp dist/mac/DS_Store.in "$TMP/.DS_Store"
chmod go-rwx "$TMP/.DS_Store"
ln -s /Applications "$TMP/Applications"
# copies the prepared bundle into the dir that will become the DMG 
cp -R "$IN" "$TMP"

# create
hdiutil create -srcfolder "$TMP" \
               -format UDZO -imagekey zlib-level=9 \
               -scrub \
               "$OUT" \
               || die "Error creating DMG :("

# done !
echo 'DMG size:' `du -hs "$OUT" | awk '{print $1}'`
