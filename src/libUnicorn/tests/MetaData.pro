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
HEADERS += ../UnicornCommon.h \
           ../UnicornDllExportMacro.h \
           ../../libMoose/MooseCommon.h \
           ../../libMoose/LastFmSettings.h \
           ../metadata.h \
           ../TrackInfo.h \
           ../Track.h \
           ../Settings.h \
           ../Logger.h \
           ../StationUrl.h \
           ../../User.h \

SOURCES += TestMetaData.cpp \
           ../UnicornCommon.cpp \
           ../../libMoose/MooseCommon.cpp \
           ../../libMoose/LastFmSettings.cpp \
           ../metadata.cpp \
           ../TrackInfo.cpp \
           ../Settings.cpp \
           ../Logger.cpp \
           ../StationUrl.cpp \
           ../md5/md5.c \
           ../../User.cpp
