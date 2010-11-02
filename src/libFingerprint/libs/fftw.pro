TEMPLATE = lib
VERSION = 1.0.0
CONFIG += dll
TARGET = fftw3f
QT -= core gui

include( ../../../definitions.pro.inc )

DESTDIR = $$BIN_DIR

mac {
	system(cd fftw/src && CC=g++ CXX=g++ ./configure --disable-dependency-tracking --enable-threads --enable-float --disable-fast-install --enable-static CFLAGS=\'-arch i386 -arch ppc\' && LDFLAGS=\'-arch i386 -arch ppc\')
	system(make -C fftw/src/)
	system(cp fftw/src/.libs/libfftw3f.a $$ROOT_DIR/res/mac/)
}

linux* {
	system(./fftw/src/configure --enable-threads --enable-float --enable-shared)
	system(make -C fftw/src/)
	system(cp fftw/src/.libs/libfftw3f.so* $$BIN_DIR)
}

