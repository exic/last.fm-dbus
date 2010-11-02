TEMPLATE = app
TARGET = killer
QT -= core gui
CONFIG -= qt
CONFIG += console

include( ../../definitions.pro.inc )

HEADERS = stdafx.h

SOURCES = stdafx.cpp \
          KillProcess.h \
          Killer.cpp

LIBS -= -lLastFmTools$$EXT
LIBS += -lshell32 -luser32
          
# Link CRT statically into exe as we need to run it from Temp
QMAKE_CXXFLAGS_RELEASE -= -MD
QMAKE_CXXFLAGS_RELEASE += -MT

# Override the QMAKE_POST_LINK from unicorn.pro.inc as we don't want a dep on the CRT
QMAKE_POST_LINK = mt.exe -manifest trustInfo.manifest \
    -outputresource:$${DESTDIR}/$${TARGET}.exe;$${LITERAL_HASH}1

# Don't want Unicode as that might not work on Win98
DEFINES -= UNICODE          

RC_FILE = Killer.rc
