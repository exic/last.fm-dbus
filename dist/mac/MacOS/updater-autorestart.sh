#/bin/bash
#Auto restarter script for OS X used after update is complete

killall LastFmHelper

while [[ -a $1 ]]; do
	sleep 0.5
done

exec ./Last.fm
