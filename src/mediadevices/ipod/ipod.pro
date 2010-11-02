TEMPLATE = lib
CONFIG += service
TARGET = Ipod_device
QT += sql

include( ../../../definitions.pro.inc )

HEADERS = IpodDevice.h
SOURCES = IpodDevice.cpp

linux* {
    LIBS += -lgpod

    INCLUDEPATH += /usr/include/gpod-1.0 /usr/include/glib-2.0 /usr/lib/glib-2.0/include
}

mac {
    LIBS += -L/sw/lib -lgpod -lglib-2.0
    INCLUDEPATH += /sw/include/gpod-1.0 /sw/include/glib-2.0 /sw/lib/glib-2.0/include
}

win32 {
    LIBS += -lgpod
    INCLUDEPATH += include include/glib /dev/include
}
