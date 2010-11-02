TEMPLATE = app
QT -= gui 
CONFIG += console

include( ../../definitions.pro.inc )

HEADERS = Cleaner.h

SOURCES = main.cpp \
          Cleaner.cpp

RC_FILE = Cleaner.rc

LIBS += -luser32
