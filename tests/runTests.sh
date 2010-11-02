#!/bin/sh

RUNDIR=`dirname $0`

find $RUNDIR/bin -type f -perm +1 | xargs ./logTest.sh
