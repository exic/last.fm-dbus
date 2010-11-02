TEMPLATE = lib
CONFIG += extension
TARGET = messengernotify
QT += gui xml network

include( ../../../definitions.pro.inc )

# We don't need Moose
LIBS -= -lMoose$$EXT 

win32:LIBS += -luser32

FORMS = settingsdialog_messenger.ui
HEADERS = messengernotifyextension.h 
SOURCES = messengernotifyextension.cpp 
RESOURCES = icons.qrc
