CONFIG += qtestlib
QT += network xml
TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += ../ ../../ ../../libMoose/

BIN_DIR = ../../../tests/bin
BUILD_DIR = ../../../tests/bin/build
DESTDIR = ../../../tests/bin

OBJECTS_DIR = $$BUILD_DIR
MOC_DIR = $$BUILD_DIR

# Input
HEADERS += ../../libMoose/MooseCommon.h \
           ../../libMoose/LastFmSettings.h \
           ../UnicornCommon.h \
           ../Settings.h \
           ../StationUrl.h \
           ../logger.h \
           ../TrackInfo.h \
           ../UnicornDllExportMacro.h

SOURCES += ../../libMoose/MooseCommon.cpp \
           ../../libMoose/LastFmSettings.cpp \
           ../UnicornCommon.cpp \
           ../Settings.cpp \
           ../StationUrl.cpp \
           ../logger.cpp \
           ../TrackInfo.cpp \
           ../md5/md5.c

HEADERS += ../WebService.h

SOURCES += TestWebService.cpp \
           ../WebService.cpp
