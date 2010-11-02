# This needs to be added here to enable a japanese build.
# DEFINES += HIDE_RADIO

win32: ROOT_DIR = $$system( cd )
else:  ROOT_DIR = $$system( pwd )
BIN_DIR = $$ROOT_DIR/bin
DESTDIR = $$BIN_DIR

CONFIG( service ) {
    CONFIG += plugin

    win32: DESTDIR = $$BIN_DIR
    else:  DESTDIR = $$BIN_DIR/services

    TARGET = srv_$$TARGET
}

CONFIG( extension ) {
    CONFIG += plugin

    win32: DESTDIR = $$BIN_DIR
    else:  DESTDIR = $$BIN_DIR/extensions

    TARGET = ext_$$TARGET
}

INCLUDEPATH += $$ROOT_DIR/src/libMoose $$ROOT_DIR/src/libUnicorn
DEPENDPATH += $$ROOT_DIR/src/libUnicorn $$ROOT_DIR/src/libFingerprint

CONFIG(debug, debug|release) {
    extension:DESTDIR = $$DESTDIR/debug
    service:DESTDIR = $$DESTDIR/debug

    # prevents the second instance just transferring control to the first
    # via the socket, which is useful if you want to run a client for fun, and
    # a client for debug -- mxcl
    #DEFINES += LASTFM_MULTI_PROCESS_HACK
}

# FIXME to use proper path
UNICORNPATH = $$ROOT_DIR/src/libUnicorn
include( src/libUnicorn/unicorn.pro.inc )

LIBS += -lMoose$$EXT

# TODO: not sure if this is right now, should probably be determined on a per-plugin basis
plugin:LIBS += -lLastFmTools$$EXT
contains( TEMPLATE, app ):LIBS += -lLastFmTools$$EXT
