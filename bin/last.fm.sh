#!/bin/sh
# executes all *nix variants of Last.fm executable --mxcl

RUNDIR=`dirname $0`
export LD_LIBRARY_PATH=$RUNDIR:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$RUNDIR:$DYLD_LIBRARY_PATH

if [ -f $RUNDIR/last.fm_debug ]
then
    `$RUNDIR/last.fm_debug $*`
elif [ -f $RUNDIR/last.fm ]
then
    `$RUNDIR/last.fm $*`
elif [ -d $RUNDIR/Last.fm_debug.app ]
then
    open $RUNDIR/Last.fm_debug.app $*
elif [ -d $RUNDIR/Last.fm.app ]
then
    open $RUNDIR/Last.fm.app $*
fi