TEMPLATE = lib
CONFIG += service
TARGET = output_portaudio

include( ../../../definitions.pro.inc )

# PortAudio = warnings-city
QMAKE_CFLAGS_WARN_ON = ""

INCLUDEPATH += PortAudio/include \
               PortAudio/common

SOURCES = portAudioOutput.cpp \
          \
          PortAudio/common/pa_skeleton.c \
          PortAudio/common/pa_process.c \
          PortAudio/common/pa_dither.c \
          PortAudio/common/pa_allocation.c \
          PortAudio/common/pa_converters.c \
          PortAudio/common/pa_cpuload.c \
          PortAudio/common/pa_front.c \
          PortAudio/common/pa_debugprint.c \
          PortAudio/common/pa_stream.c \
          PortAudio/common/pa_trace.c \
   
HEADERS = portAudioOutput.h


unix:linux-g++ {
    INCLUDEPATH += PortAudio/os/unix

    DEFINES += PA_USE_ALSA \
               PA_USEOSS
        
    SOURCES += PortAudio/hostapi/alsa/pa_linux_alsa.c \
               PortAudio/hostapi/oss/recplay.c \
               PortAudio/hostapi/oss/pa_unix_oss.c \
               PortAudio/os/unix/pa_unix_util.c \
               PortAudio/os/unix/pa_unix_hostapis.c
    
    LIBS += -lasound \
            -lrt \
            -lm
}

unix:mac {
    INCLUDEPATH += PortAudio/os/mac_osx

    DEFINES += PA_USE_COREAUDIO
    
    SOURCES += PortAudio/os/unix/pa_unix_util.c \
               PortAudio/hostapi/coreaudio/ringbuffer.c \
               PortAudio/hostapi/coreaudio/pa_mac_core.c \
               PortAudio/hostapi/coreaudio/pa_mac_core_blocking.c \
               PortAudio/hostapi/coreaudio/pa_mac_core_utilities.c \
               PortAudio/common/pa_ringbuffer.c \
               PortAudio/os/mac_osx/pa_mac_hostapis.c

    LIBS += -framework AudioToolbox \
            -framework AudioUnit \
            -framework CoreAudio \
            -framework CoreServices
}

win32 {
    INCLUDEPATH += PortAudio/common PortAudio/os/win \
                   ../../rtaudioplayback/dsound
    
    DEFINES += PA_USE_DSOUND \
               PA_NO_WMME \
               PA_NO_ASIO
    
    SOURCE += PortAudio/os/win/pa_win_hostapis.c \ 
              PortAudio/os/win/pa_win_util.c \
              PortAudio/hostapi/dsound/pa_win_ds.c \
              PortAudio/hostapi/dsound/pa_win_ds_dynlink.c
    
    LIBS += -Ldsound -ldsound \
            -lwinmm \
            -lole32
}
