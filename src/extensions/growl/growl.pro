TEMPLATE = lib
CONFIG += extension
TARGET = growlextension
QT = core gui

include( ../../../definitions.pro.inc )

LIBS += -framework Carbon
HEADERS += growlextension.h
SOURCES += growlextension.cpp
