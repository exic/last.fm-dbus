TEMPLATE = lib
CONFIG += dll
VERSION = 1.0.0
TARGET = LastFmTools
QT += xml network gui sql

# HACK! FIXME. This only works in the context of the client. The ROOT_DIR
# needs to be configurable by the project unicorn is being included in.
ROOT_DIR = ../..
BIN_DIR = $$ROOT_DIR/bin
DESTDIR = $$BIN_DIR
UNICORNPATH = .
include( unicorn.pro.inc )

win32 {
    DEFINES += UNICORN_DLLEXPORT_PRO
    LIBS += -lshell32

    HEADERS += UnicornCommonWin.h
    SOURCES += UnicornCommonWin.cpp
}

mac {
    LIBS = -framework Carbon -framework SystemConfiguration -framework CoreServices $$LIBS

    HEADERS += UnicornCommonMac.h AppleScript.h
    SOURCES += UnicornCommonMac.cpp AppleScript.cpp
}

INCLUDEPATH += .

HEADERS += \
        URLLabel.h \
        draglabel.h \
        CachedHttp.h \
        CachedHttpJanitor.h \
        RedirectHttp.h \
        imagebutton.h \
        metadata.h \
        md5/md5.h \
        TrackInfo.h \
        watermarkwidget.h \
        logger.h \
        Settings.h \
        WebService.h \
        WebService/fwd.h \
        WebService/Request.h \
        WebService/GetXspfPlaylistRequest.h \
        WebService/FingerprintQueryRequest.h \
        WebService/SubmitFullFingerprintRequest.h \
        WebService/TrackUploadRequest.h \
        WebService/UserLabelsRequest.h \
        WebService/XmlRpc.h \
        WebService/FrikkinNormanRequest.h \
        LastMessageBox.h \
        StationUrl.h \
        StopWatch.h \
        UnicornCommon.h \
        Track.h \
        Station.h \
        WeightedStringList.h \
        DragMimeData.h \
        Collection.h \
        mbid_mp3.h


SOURCES += \
        CachedHttp.cpp \
        CachedHttpJanitor.cpp \
        RedirectHttp.cpp \
        metadata.cpp \
        URLLabel.cpp \
        draglabel.cpp \
        imagebutton.cpp \
        md5/md5.c \
        TrackInfo.cpp \
        watermarkwidget.cpp \
        logger.cpp \
        Settings.cpp \
        WebService.cpp \
        WebService/Request.cpp \
        WebService/XmlRpc.cpp \
        WebService/ActionRequest.cpp \
        WebService/ArtistMetaDataRequest.cpp \
        WebService/ArtistTagsRequest.cpp \
        WebService/AlbumTagsRequest.cpp \
        WebService/ChangeStationRequest.cpp \
        WebService/DeleteFriendRequest.cpp \
        WebService/FriendsRequest.cpp \
        WebService/Handshake.cpp \
        WebService/NeighboursRequest.cpp \
        WebService/RecentTrackRequest.cpp \
        WebService/ReportRebufferingRequest.cpp \
        WebService/RecommendRequest.cpp \
        WebService/SearchTagsRequest.cpp \
        WebService/SetTagRequest.cpp \
        WebService/SimilarArtistsRequest.cpp \
        WebService/SimilarTagsRequest.cpp \
        WebService/FingerprintQueryRequest.cpp \
        WebService/SubmitFullFingerprintRequest.cpp \
        WebService/UserLabelsRequest.cpp \
        WebService/UserPicturesRequest.cpp \
        WebService/UserTagsRequest.cpp \
        WebService/TopTagsRequest.cpp \
        WebService/TrackMetaDataRequest.cpp \
        WebService/TrackToIdRequest.cpp \
        WebService/TrackTagsRequest.cpp \
        WebService/TrackUploadRequest.cpp \
        WebService/VerifyUserRequest.cpp \
        WebService/GetXspfPlaylistRequest.cpp \
        WebService/ProxyTestRequest.cpp \
        WebService/FrikkinNormanRequest.cpp \
        LastMessageBox.cpp \
        StationUrl.cpp \
        StopWatch.cpp \
        UnicornCommon.cpp \
        DragMimeData.cpp \
        Collection.cpp
