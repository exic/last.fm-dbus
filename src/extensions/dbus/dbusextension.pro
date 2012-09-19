TEMPLATE = lib
CONFIG += extension qdbus
TARGET = dbusextension
QT += gui xml

include( ../../../definitions.pro.inc )

# We don't need Moose
LIBS -= -lMoose$$EXT

# If the build fails, make sure this is where libLastFmTools.so resides
LIBS += -L/usr/lib/lastfm
# I found it necessary to do this on my Debian system:
#LIBS -= -lLastFmTools$$EXT
#LIBS += -l:libLastFmTools.so.1

HEADERS = DBusExtension.h TrackListAdaptor.h RootAdaptor.h PlayerAdaptor.h
SOURCES = DBusExtension.cpp TrackListAdaptor.cpp RootAdaptor.cpp PlayerAdaptor.cpp
