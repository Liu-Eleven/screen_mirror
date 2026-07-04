
cmake_minimum_required(VERSION 2.8.1)

include_directories(
	${TINA_ROOT_PATH}/platform/allwinner/multimedia/libcedarc/include
	${TINA_ROOT_PATH}/platform/allwinner/multimedia/libcedarx/libcore/base/include
	${TINA_ROOT_PATH}/platform/allwinner/multimedia/libcedarx/libcore/common/iniparser
	${TINA_ROOT_PATH}/platform/allwinner/multimedia/libcedarx/libcore/playback/include
	${TINA_ROOT_PATH}/platform/allwinner/multimedia/libcedarx/external/include/adecoder
        ./
	../libwfd2
	../WFDPlayer/tsdemux
	../WFDPlayer/playback/include
	)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

add_executable( wfd_test
	ut_wfdplayer.cpp
        )

target_link_libraries( wfd_test wfd_rtsp wfd_player wpa_client )

