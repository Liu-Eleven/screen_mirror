#
# Makefile
#
CC ?= gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
LDFLAGS ?= -lm -lasound -lwfd_player -lwfd2 -lwpa_client
SCREENMIRROR_DIR ?= $(abspath $(LVGL_DIR)/../../screenmirror_lib)
LDFLAGS += -L$(SCREENMIRROR_DIR)/build -lscreenmirror -pthread
BIN = lv_projector

#include
INCLUDES = -I$(LVGL_DIR)/ \
            -I$(LVGL_DIR)/include \
            -I$(LVGL_DIR)/widget \
            -I$(LVGL_DIR)/Setting \
            -I$(LVGL_DIR)/System \
            -I$(LVGL_DIR)/System/ota \
            -I$(LVGL_DIR)/Common \
            -I$(LVGL_DIR)/Common/setting \
            -I$(LVGL_DIR)/Common/language/string \
            -I$(LVGL_DIR)/AWCast \
            -I$(LVGL_DIR)/USBCast \
            -I$(LVGL_DIR)/ProjectorAdapter \
            -I$(LVGL_DIR)/Layer \
            -I$(LVGL_DIR)/WiredSP \
            -I$(LVGL_DIR)/WirelessSP \
            -I$(LVGL_DIR)/Factory_test \
            -I$(SCREENMIRROR_DIR)/include \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libtmedia/tplayer/awsink/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libtmedia/tplayer/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/base/include/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/xplayer/include \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/playback/include/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/external/include/adecoder/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/display/libuapi/src/include \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarc/include \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/parser/include/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/multimedia/libcedarx/libcore/stream/include/ \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/common/libkey/src/include \
            -I$(LICHEE_TOP_DIR)/platform/allwinner/display/display2/hdmirx/libhdmiapi/include

#Collect the files to compile
MAINSRC = main.c lv_pro_launcher.c
SRCDIRS = $(LVGL_DIR)/widget \
        $(LVGL_DIR)/USBCast \
        $(LVGL_DIR)/Common \
        $(LVGL_DIR)/Layer \
        $(LVGL_DIR)/Network \
        $(LVGL_DIR)/Setting \
        $(LVGL_DIR)/Source \
        $(LVGL_DIR)/Media \
        $(LVGL_DIR)/WiredSP \
        $(LVGL_DIR)/WirelessSP \
        $(LVGL_DIR)/System \
        $(LVGL_DIR)/Bluetooth \
        $(LVGL_DIR)/ProjectorAdapter \
        $(LVGL_DIR)/Factory_test

ALL_SUB_DIRS  =  $(shell find $(SRCDIRS) -maxdepth 3 -type d)
MAINSRC += $(foreach dir, $(ALL_SUB_DIRS), $(wildcard $(dir)/*.c))

ifeq ($(PROJECTOR_RESOLUTION), LV_USE_RESOLUTION_1080P)
MAINSRC += $(shell find -L $(LVGL_DIR)/img_res/1080p -name \*.c)
else
MAINSRC += $(shell find -L $(LVGL_DIR)/img_res/720p -name \*.c)
endif

MAINSRC += $(wildcard $(LVGL_DIR)/AWCast/*.c)

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

#Do not compile the example
EXCSRCS += $(shell find -L $(LVGL_DIR)/$(LVGL_DIR_NAME)/examples -name \*.c)
CSRCS := $(filter-out $(EXCSRCS),$(CSRCS))

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

AWCAST_LIB = libwfd_player.so libwpa_client.so libwfd2.so
## MAINOBJ -> OBJFILES

all: default

$(AWCAST_LIB):
	@echo "Building $@ in $(LVGL_DIR)/AWCast/src"
	$(MAKE) -C $(LVGL_DIR)/AWCast/src \
			ARCH="$(ARCH)" \
			AR="$(AR)" \
			CC="$(CC)" \
			CPP="$(CPP)" all

%.o: %.c
	@$(CC)  $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "CC $<"

default: $(AOBJS) $(COBJS) $(MAINOBJ) $(AWCAST_LIB)
	$(MAKE) -C $(SCREENMIRROR_DIR)
	$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS) $(INCLUDES)

clean:
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ)
	$(MAKE) -C $(LVGL_DIR)/AWCast/src  \
			ARCH="$(ARCH)" \
			AR="$(AR)" \
			CC="$(CC)" \
			CPP="$(CPP)" \
			clean
