TARGET := libwfd_player.so

C_SRCS := tsdemux/tsdemux.c \
	tsdemux/ts_prog_parser.c \
	tsdemux/ts_buffer.c \
	playback/videoRenderComponent_newDisplay.c \
	playback/audioRenderComponent.c \
	playback/audioDecComponent.c \
	playback/avtimer.c \
	playback/streamManager.c \
	playback/videoDecComponent.c \
	playback/player.c \
	playback/baseComponent.c \
	uibc/miracle-uibcctl.c

CPP_SRCS := WFDPlayer2.cpp

OBJS := $(C_SRCS:.c=.o) $(CPP_SRCS:.cpp=.o)

INCLUDE_PATHS := -I./ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarc/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/base/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/common/iniparser \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/playback/include/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include/adecoder \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include/wolfssl/ \
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/wfd_player/tsdemux \
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/wfd_player/playback	\
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/wfd_player/playback/include \
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/libwfd2 \
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/wfd_hdcp/ \
				-I$(LICHEE_TOP_DIR)/platform/thirdparty/gui/lvgl-8/lv_projector/src/AWCast/src/wfd_sink/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libtmedia/tplayer/awsink/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libtmedia/tplayer/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/xplayer/include/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/parser/include/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/stream/include/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/parser/ts/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/display/libuapi/src/include/ \
				-I../../include

TINA_LINUX_API := "1"
LOCAL_CXXFLAGS := -DCONF_SUPPORT_G2D -DTINA_LINUX_SUPPORT=$(TINA_LINUX_API)
LOCAL_CFLAGS := -DCONF_SUPPORT_G2D -DTINA_LINUX_SUPPORT=$(TINA_LINUX_API)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -fPIC -shared -o $@ $(OBJS) $(LIB_PATH) $(LIBS)

%.o: %.cpp
	echo "$(CC) -fPIC $(LOCAL_CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $< "
	$(CC) -fPIC $(LOCAL_CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $< 

%.o: %.c
	echo "$(CC) -fPIC -shared $(CFLAGS) $(LOCAL_CFLAGS) $(INCLUDE_PATHS) -o $@ -c $<"
	$(CC) -fPIC -shared $(CFLAGS) $(LOCAL_CFLAGS) $(INCLUDE_PATHS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
