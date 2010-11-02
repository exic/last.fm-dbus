TEMPLATE = lib
CONFIG += extension
TARGET = skypenotify
QT += gui

include( ../../../definitions.pro.inc )

win32:LIBS += -luser32

FORMS = settingsdialog_skype.ui
HEADERS = skypenotifyextension.h skypenotifyextension_win.h
SOURCES = skypenotifyextension.cpp skypenotifyextension_win.cpp
RESOURCES = icons.qrc
