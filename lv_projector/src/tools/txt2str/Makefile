
toolprefix ?= x86_64-linux-gnu
SECURE_STORAGE_EN ?= n
SECURE_STORAGE_LIB_PATH ?=
SECURE_STORAGE_INC_PATH ?=
STATIC_EN ?= n

CC     := ${toolprefix}-gcc
CPP    := ${toolprefix}-g++
STRIP  := ${toolprefix}-strip
CFLAGS := -g -Wall --static -Wno-unused-variable -Wno-unused-but-set-variable -Wno-sign-compare
CPPFLAGS := -g -Wall -fPIC
CPPFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden -Wno-sign-compare
LOCAL_FLAGS += -lstdc++

STATIC_EN = y
ifeq ($(STATIC_EN), y)
	CPPFLAGS += --static
endif


ALL: txt2str

%.o: %.c
	@echo "compile $< ..."
	@$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	@echo "compile $< ..."
	@$(CPP) $(CPPFLAGS) ${INCLUDES} $(LOCAL_FLAGS) -c -o $@ $<



txt2str: main.o
	@$(CPP) $(CPPFLAGS) ${INCLUDES} $(LOCAL_FLAGS) -o $@ $^
	@$(STRIP) $@

clean:
	rm -rf ${TARGET} *.o txt2str

ROOTFS := ${OUTPUT_DIR}/test

install:
