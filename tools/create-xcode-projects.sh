#!/bin/bash

# run from the parent directory!
# @author <max@last.fm>

echo '==> Creating xcodeproj files'

files=`find src -name \*.pro`

for x in $files
do
		pushd `dirname $x`
		qmake -spec macx-xcode
		popd &> /dev/null
done

# create the root project
qmake -spec macx-xcode

echo '==> Fixing broken Makefiles'

# qmake is broken, and uses relative and absolute paths, and thus the build won't work
find . -name qt_preprocess.mak -o -name qt_makeqmake.mak | xargs perl -pi -e "s:(^|\s)(\.\./)+build:\1$PWD/build:g"
