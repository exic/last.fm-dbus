TEMPLATE = app
TARGET = LastFmHelper
QT += gui network xml sql

CONFIG -= app_bundle

include( ../../definitions.pro.inc )

HEADERS = mediadevicewatcher.h \
          controlinterface.h

SOURCES = main.cpp \
          mediadevicewatcher.cpp \
          controlinterface.cpp

breakpad {
    LIBS += -lbreakpad$$EXT
}

win32 {
	LIBS += -lshfolder -luser32 -lshell32 -lversion

	RC_FILE = Helper.rc
}

unix {
#	QT -= gui
#	QT += console
}

mac {
  LIBS += -framework Carbon
}

linux* {
  INCLUDEPATH += breakpad
}
