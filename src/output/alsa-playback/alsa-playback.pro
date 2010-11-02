TEMPLATE = lib
TARGET = output_alsa
CONFIG += service
QT += gui xml network

LIBS += -lasound
QMAKE_CFLAGS_WARN_OFF = -w
QMAKE_CFLAGS_WARN_ON =

include( ../../../definitions.pro.inc )

HEADERS = alsaplayback.h alsaaudio.h xconvert.h
SOURCES = alsaplayback.cpp alsaaudio.cpp xconvert.c
