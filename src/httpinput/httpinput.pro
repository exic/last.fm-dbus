TEMPLATE = lib
CONFIG += service
TARGET = httpinput
QT += network gui xml

include( ../../definitions.pro.inc )

HEADERS = httpinput.h
SOURCES = httpinput.cpp
