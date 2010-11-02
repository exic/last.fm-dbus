#!/bin/bash
[[ -f Makefile ]] && make clean > /dev/null
rm -rf build
find . -name "Makefile" -delete
cd bin && perl ../tools/svn-clean.pl
