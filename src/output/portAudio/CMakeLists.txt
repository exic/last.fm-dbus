project( tomahawk )
#find_package( Qt4 REQUIRED )

include( ${QT_USE_FILE} )
add_definitions( ${QT_DEFINITIONS} )
add_definitions( -DQT_PLUGIN )
add_definitions( -DQT_NO_DEBUG )
add_definitions( -DQT_SHARED )

if(UNIX AND NOT APPLE)
add_definitions( -DPA_LITTLE_ENDIAN -DPA_USE_ALSA=1 -DPA_USEOSS=1 )

set( portAudioOutputSources
        portAudioOutput.cpp

	./PortAudio/common/pa_skeleton.c
	./PortAudio/common/pa_process.c
	./PortAudio/common/pa_dither.c
	./PortAudio/common/pa_allocation.c
	./PortAudio/common/pa_converters.c
	./PortAudio/common/pa_cpuload.c
	./PortAudio/common/pa_front.c
	./PortAudio/common/pa_debugprint.c
	./PortAudio/common/pa_stream.c
	./PortAudio/common/pa_trace.c

	./PortAudio/hostapi/alsa/pa_linux_alsa.c
	./PortAudio/hostapi/oss/recplay.c
	./PortAudio/hostapi/oss/pa_unix_oss.c
	./PortAudio/os/unix/pa_unix_util.c
	./PortAudio/os/unix/pa_unix_hostapis.c
)

include_directories( . ../../interfaces ../../tomahawkLib
	PortAudio/include PortAudio/common PortAudio/os/unix
	${QT_INCLUDE_DIR}
)
endif(UNIX AND NOT APPLE)

if(APPLE)
add_definitions( -DPA_LITTLE_ENDIAN -DPA_USE_COREAUDIO=1 )

set( portAudioOutputSources
        portAudioOutput.cpp

	./PortAudio/common/pa_skeleton.c
	./PortAudio/common/pa_process.c
	./PortAudio/common/pa_dither.c
	./PortAudio/common/pa_allocation.c
	./PortAudio/common/pa_converters.c
	./PortAudio/common/pa_cpuload.c
	./PortAudio/common/pa_front.c
	./PortAudio/common/pa_debugprint.c
	./PortAudio/common/pa_stream.c
	./PortAudio/common/pa_trace.c

	./PortAudio/os/unix/pa_unix_util.c
	
	./PortAudio/hostapi/coreaudio/ringbuffer.c
	./PortAudio/hostapi/coreaudio/pa_mac_core.c
	./PortAudio/hostapi/coreaudio/pa_mac_core_blocking.c
	./PortAudio/hostapi/coreaudio/pa_mac_core_utilities.c
	./PortAudio/os/mac_osx/pa_mac_hostapis.c
)

include_directories( . ../../interfaces ../../tomahawkLib
	PortAudio/include PortAudio/common PortAudio/os/mac_osx
	${QT_INCLUDE_DIR}
)
endif(APPLE)

if(WIN32)
add_definitions( -Ldsound -DPA_USE_DSOUND=1 -DPA_NO_WMME=1 -DPA_NO_ASIO=1 )

set( portAudioOutputSources
        portAudioOutput.cpp

	./PortAudio/common/pa_skeleton.c
	./PortAudio/common/pa_process.c
	./PortAudio/common/pa_dither.c
	./PortAudio/common/pa_allocation.c
	./PortAudio/common/pa_converters.c
	./PortAudio/common/pa_cpuload.c
	./PortAudio/common/pa_front.c
	./PortAudio/common/pa_debugprint.c
	./PortAudio/common/pa_stream.c
	./PortAudio/common/pa_trace.c

	./PortAudio/os/win/pa_win_hostapis.c
	./PortAudio/os/win/pa_win_util.c

	./PortAudio/hostapi/dsound/pa_win_ds.c
	./PortAudio/hostapi/dsound/pa_win_ds_dynlink.c
)

include_directories( . ../../interfaces ../../tomahawkLib
	PortAudio/include PortAudio/common PortAudio/os/win
	dsound
	${QT_INCLUDE_DIR}
)
endif(WIN32)

set( portAudioOutputHeaders
        portAudioOutput.h
)

qt4_wrap_cpp( portAudioOutputMoc ${portAudioOutputHeaders} )
add_library( output_portAudio SHARED ${portAudioOutputSources} ${portAudioOutputMoc} )

if(UNIX AND NOT APPLE)
target_link_libraries( output_portAudio tomahawkLib
	asound rt m
	${QT_LIBRARIES}
)
endif(UNIX AND NOT APPLE)

if(APPLE)
target_link_libraries( output_portAudio tomahawkLib

	"-framework AudioToolbox"
	"-framework AudioUnit"
	"-framework CoreAudio"

	${QT_LIBRARIES}
)
endif(APPLE)

if(WIN32)
target_link_libraries( output_portAudio tomahawkLib
    dsound winmm ole32
	${QT_LIBRARIES}
)
endif(WIN32)

install( TARGETS output_portAudio DESTINATION bin/services )
