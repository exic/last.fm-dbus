TEMPLATE = lib
VERSION = 1.0.0
CONFIG += dll
TARGET = Moose
QT += xml network gui

include( ../../definitions.pro.inc )

FORMS += \
        confirmdialog.ui

HEADERS += \
        confirmdialog.h \
        MooseCommon.h \
        LastFmSettings.h

SOURCES += \
        confirmdialog.cpp \
        MooseCommon.cpp \
        LastFmSettings.cpp

mac {
    SOURCES += QtOverrides/SystemTrayIcon.cpp
}


win32 {
    DEFINES += MOOSE_DLLEXPORT_PRO
    LIBS += -lshell32
}

# This is added in definitions.pro.inc, get rid of it
LIBS -= -lMoose$$EXT
LIBS += -lLastFmTools$$EXT
