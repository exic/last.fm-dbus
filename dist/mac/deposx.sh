#!/bin/sh
# author: max@last.fm, chris@last.fm
################################################################################


if [ -z $QTDIR ]
then
    echo QTDIR must be set
    exit 1
fi

cd Contents

QTLIBS=`ls Frameworks | cut -d. -f1`
LIBS=`cd MacOS && ls -fR1 | grep dylib`
################################################################################


function deposx_change 
{
    echo "D \`$1'"

    for y in $QTLIBS
    do
        install_name_tool -change $QTDIR/lib/$y.framework/Versions/4/$y \
                          @executable_path/../Frameworks/$y.framework/Versions/4/$y \
                          "$1"
    done
    
    for y in $LIBS
    do
        install_name_tool -change $y \
                          @executable_path/$y \
                          "$1"
    done
}
################################################################################


# first all libraries and executables
find MacOS -type f -a -perm -100 | while read x
do
    y=$(file "$x" | grep 'Mach-O')
    test -n "$y" && deposx_change "$x"
done

# now Qt
for x in $QTLIBS
do
    deposx_change Frameworks/$x.framework/Versions/4/$x
    install_name_tool -id @executable_path/../Frameworks/$x.framework/Versions/4/$x \
                      Frameworks/$x.framework/Versions/4/$x
done
