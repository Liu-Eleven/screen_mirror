
######### UT WFDPlayer #########
bin_PROGRAMS = ut_wfdplayer

ut_wfdplayer_SOURCES = \
    ut_wfdplayer.cpp

ut_wfdplayer_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/libwfd2 \
    -I$(srcdir) \
    -I$(top_srcdir)/WFDPlayer/platform/include \
    -I$(top_srcdir)/WFDPlayer/tsdemux \
    -I$(top_srcdir)/WFDPlayer/playback \
    -I$(top_srcdir)/WFDPlayer/playback/include \
    -I$(top_srcdir)/WFDPlayer \
    -I$(srcdir)/../WFDSink \
    -I$(srcdir)/../WFDSink/BasicUsageEnvironment/include \
    -I$(srcdir)/../WFDSink/liveMedia/include \
    -I$(srcdir)/../WFDSink/groupsock/include \
    -I$(srcdir)/../WFDSink/UsageEnvironment/include \
	-I$(STAGING_DIR)/target/usr/include/allwinner/include \
	-I$(STAGING_DIR)/target/usr/include/allwinner \
	-I$(STAGING_DIR)/target/usr/include

ut_wfdplayer_LDFLAGS = $(WFD_COMMON_LDFLAGS)

ut_wfdplayer_LDFLAGS += -lpthread \
    -L$(STAGING_DIR)/target/usr/lib \
    -lcdx_base  -lcdx_common \
    -lcdc_base  -lMemAdapter -lvdecoder \
    -lvideoengine -lVE \
    -ladecoder -lsubdecoder -lssl -lcrypto -lz \
    -lcdx_sink_tina -lasound -luci -lubox

if H133
ut_wfdplayer_LDFLAGS += -lfbm -laftertreatment -lscaledown -lsbm
endif

ut_wfdplayer_LDADD = \
    $(top_srcdir)/WFDSink/libwfd_rtsp.la \
    $(top_srcdir)/WFDPlayer/libwfd_player.la \
    $(top_srcdir)/wpa_client/libwpa_client.la \
    $(top_srcdir)/WFDBase/libwfd_base.la

