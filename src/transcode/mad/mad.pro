TEMPLATE = lib
CONFIG += service
TARGET = madtranscode
QT += gui xml network

include( ../../../definitions.pro.inc )

win32 {
    QMAKE_LFLAGS += /NODEFAULTLIB:libcmt
}

!linux* {
    LIBPATH += $$ROOT_DIR/res/mad
	INCLUDEPATH += $$ROOT_DIR/res/mad
}

LIBS += -lmad

HEADERS = madtranscode.h RingBuffer.h
SOURCES = madtranscode.cpp RingBuffer.cpp
