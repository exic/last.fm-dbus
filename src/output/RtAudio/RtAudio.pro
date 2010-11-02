TEMPLATE = lib
CONFIG += service
TARGET = rtaudioplayback
QT += gui xml network

include( ../../../definitions.pro.inc )

HEADERS = rtaudioplayback.h
SOURCES = rtaudioplayback.cpp rtaudio/RtAudio.cpp

unix:!mac {
    LIBS += -lasound
}

win32 {
   LIBS += -lwinmm -Ldsound -ldsound -lole32 -lgdi32 -luser32
   INCLUDEPATH += dsound
}

mac {
   LIBS += -framework CoreAudio
}
