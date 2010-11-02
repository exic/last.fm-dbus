TEMPLATE = lib
CONFIG += service
TARGET = itunesdevice
QT += network gui xml sql

include( ../../../definitions.pro.inc )

win32:LIBS += -lshfolder

HEADERS = itunesdevice.h
SOURCES = itunesdevice.cpp
