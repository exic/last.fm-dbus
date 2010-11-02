TEMPLATE = lib
CONFIG += service
TARGET = mp3transcode
QT += gui xml network

include( ../../../definitions.pro.inc )

win32:LIBS += -lshfolder

HEADERS = mp3transcode.h
SOURCES = mp3transcode.cpp \
        \
        mpglib/common.c mpglib/dct64_i386.c mpglib/decode_i386.c \
        mpglib/interface.c mpglib/layer1.c mpglib/layer2.c \
        mpglib/layer3.c mpglib/tabinit.c
