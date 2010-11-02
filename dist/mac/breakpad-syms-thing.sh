# @author Max Howell <max@last.fm>

if [[ $1 == '' ]]
then
    echo "Brief: Uploads new symbols to oops.last.fm"
    echo "Usage: $0 [path to libs]"
    exit 0
fi

rm -rf build/syms
mkdir -p build/syms
files=`find $1 -type f -a -perm -100`
# stolen from mozilla Makefile.in
echo $files | xargs file -L \
            | grep "Mach-O" \
            | grep -v "for architecture" \
            | cut -f1 -d':' \
            | xargs dist/breakpad-make-symbolstore.pl \
	                -a "ppc i386" \
	                $LASTFM_CWD/build/dump_syms \
	                build/syms

rm -f build/symbols.tar.bz2

cd build/syms
if [ -z "`ls`" ]; then
    echo "No symbols!"
    exit 0
fi

tar cjf ../symbols.tar.bz2 .

curl -T build/symbols.tar.bz2 http://oops.last.fm/symbols/upload
