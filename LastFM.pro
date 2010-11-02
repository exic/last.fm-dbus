TEMPLATE = subdirs

# build as ordered or build will fail
CONFIG += ordered


SUBDIRS = src/libUnicorn \
          src/libMoose \
          src/CrashReporter \
          src/breakpad \
          src/libFingerprint/fplib/pro_qmake/fplib.pro \
          src/libFingerprint/ \
          src \
          src/httpinput \
          src/mediadevices/ipod \
          src/transcode/mad \
          src/Twiddly

win32 {
    SUBDIRS += src/extensions/skype \
               src/extensions/messenger \
               src/output/RtAudio
               
    SUBDIRS -= src/mediadevices/ipod
               
    CONFIG( release, release|debug ) {
		SUBDIRS += src/Updater \
				   src/Killer \
                   src/Cleaner
    }
}

mac {
    SUBDIRS += src/extensions/growl \
               src/output/portAudio
               
    SUBDIRS -= src/mediadevices/ipod
}

linux* {
    SUBDIRS -= src/Twiddly \
               src/Bootstrapper/ITunesDevice

    SUBDIRS += src/output/alsa-playback \
               src/output/portAudio
}


!breakpad {
    # we have to remove rather than add as these things must be built before the main application
    SUBDIRS -= src/CrashReporter src/breakpad
}
