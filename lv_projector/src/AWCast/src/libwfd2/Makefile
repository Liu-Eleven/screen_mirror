TARGET := libwfd2.so

C_SRCS := wpa_cli.c \
		ini_config.c \
		WFDMessageQueue.c \
		./tina_sink/subtitleControl_null.c \
		./tina_sink/layerControl_de2.c \
		./tina_sink/disputils2.c \
		./tina_sink/mixer.c \
		./tina_sink/volume_uci_config_api.c \
		./tina_sink/CdxIon.c

CPP_SRCS :=	miracast2.cpp

OBJS := $(C_SRCS:.c=.o) $(CPP_SRCS:.cpp=.o)

INCLUDE_PATHS := -I ./ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarc/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/base/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/common/iniparser \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/playback/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include/adecoder \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libaudioroute/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libtmedia/trecorder/ \
				-I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/demo/xplayerdemo \
				-I./include/ \
				-I./tina_sink/ \
				-I../wpa_client/include \
				-I../wfd_player \
				-I../wfd_player/playback/include \
				-I../../include \

TINA_LINUX_API := "1"
LOCAL_CXXFLAGS := 
LOCAL_CFLAGS := -MMD -O2 -Wall -g -DCONFIG_CTRL_IFACE -DCONFIG_CTRL_IFACE_UNIX -DCONFIG_P2P -DCONFIG_WIFI_DISPLAY \
				-DCONFIG_WPS -DCONFIG_WPS_NFC -DCONFIG_AP -DCONFIG_HS20 -DCONFIG_AUTOSCAN -DCONFIG_INTERWORKING -DIEEE8021X_EAPOL \
				-DCONF_SUPPORT_G2D -DTINA_LINUX_SUPPORT=$(TINA_LINUX_API) -DUSE_TINYALSA=1

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -fPIC -shared -o $@ $(OBJS) $(LIB_PATH) $(LIBS)

%.o: %.cpp
	$(CC) -fPIC $(LOCAL_CXXFLAGS) $(INCLUDE_PATHS) -c -o $@ $< 

%.o: %.c
	$(CC) -fPIC $(CFLAGS) $(LOCAL_CFLAGS) $(INCLUDE_PATHS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
