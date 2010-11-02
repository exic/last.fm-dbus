TEMPLATE = lib
VERSION = 1.0.0
CONFIG += dll
TARGET = LastFmFingerprint
QT += xml network sql

include( ../../definitions.pro.inc )

## definitions.pro.inc puts libs in a debug subfolder, but dont do that
DESTDIR = $$BIN_DIR

INCLUDEPATH +=	\
        fplib/include \
        ../src/ \
        $$ROOT_DIR/res/mad

HEADERS += \
        MP3_Source_Qt.h \
        Fingerprinter2.h \
        FingerprintCollector.h \
        FingerprintQueryer.h

SOURCES += \
        Sha256File.cpp \
        Sha256.cpp \
        MP3_Source_Qt.cpp \
        Fingerprinter2.cpp \
        FingerprintCollector.cpp \
        FingerprintQueryer.cpp

LIBS += -L$$BIN_DIR -lLastFmTools$$EXT

unix:mac {
    system(ranlib ../../res/mac/libfftw3f.a)
    LIBS += $$ROOT_DIR/build/fplib/libfplib$${EXT}.a -lm \
            $$ROOT_DIR/res/mac/libfftw3f.a \
            -L$$ROOT_DIR/res/libsamplerate -lsamplerate \
            -lmad
}

unix:!mac {
    LIBPATH += $$BUILD_DIR/../fplib
    LIBS += $$ROOT_DIR/build/fplib/libfplib$${EXT}.a -lsamplerate -lfftw3f -lmad
}

win32 {
    # Really not sure about the sanity of this...
    LIBPATH += $$BUILD_DIR/../fplib $$ROOT_DIR/res/libsamplerate $$ROOT_DIR/res/mad
    LIBS += -lfplib$$EXT -llibfftw3f-3 -lmad
    LIBS += -llibsamplerate

    DEFINES += __NO_THREAD_CHECK FINGERPRINT_DLLEXPORT_PRO

    # Remove warnings in debug build
    QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib
}
