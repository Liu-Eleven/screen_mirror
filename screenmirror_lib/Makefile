#
# libscreenmirror Makefile
#
# Local (host) build:
#   make all
#
# H133 cross-compile:
#   make all H133_SYSROOT=/path/to/h133-sysroot \
#            CROSS_COMPILE=arm-linux-gnueabihf- \
#            ENABLE_WIFI=1 ENABLE_BT=1 ENABLE_MIRACAST=1 \
#            ENABLE_DLNA=1 ENABLE_AIRPLAY=1 ENABLE_WIRED=1
#

CC ?= gcc
AR ?= ar

# Cross-compiler support
ifdef CROSS_COMPILE
    CC := $(CROSS_COMPILE)gcc
    AR := $(CROSS_COMPILE)ar
endif

CC_FLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-function -Wno-unused-parameter
LIB_NAME = libscreenmirror
LIB_VERSION_MAJOR = 2
LIB_VERSION_MINOR = 0
LIB_VERSION_PATCH = 0

# Paths
LIB_DIR     = .
INCLUDE_DIR = $(LIB_DIR)/include
SRC_DIR     = $(LIB_DIR)/src
BUILD_DIR   = $(LIB_DIR)/build
OBJ_DIR     = $(BUILD_DIR)/obj
DEP_DIR     = $(BUILD_DIR)/dep

# Output libraries
STATIC_LIB        = $(BUILD_DIR)/$(LIB_NAME).a
SHARED_LIB        = $(BUILD_DIR)/$(LIB_NAME).so.$(LIB_VERSION_MAJOR).$(LIB_VERSION_MINOR).$(LIB_VERSION_PATCH)
SHARED_LIB_MAJOR  = $(BUILD_DIR)/$(LIB_NAME).so.$(LIB_VERSION_MAJOR)
SHARED_LIB_BASE   = $(BUILD_DIR)/$(LIB_NAME).so

# ── Source files ──────────────────────────────────────────────────────────

CORE_SRCS = \
    $(SRC_DIR)/core/mirror_engine.c \
    $(SRC_DIR)/core/event_system.c \
    $(SRC_DIR)/core/state_machine.c

CONNECTIVITY_SRCS = \
    $(SRC_DIR)/connectivity/wifi_core.c \
    $(SRC_DIR)/connectivity/bt_core.c

PROTOCOL_SRCS = \
    $(SRC_DIR)/protocols/miracast/miracast.c \
    $(SRC_DIR)/protocols/airplay/airplay.c \
    $(SRC_DIR)/protocols/dlna/dlna.c \
    $(SRC_DIR)/protocols/wired/wired.c \
    $(SRC_DIR)/protocols/wireless/wireless.c

NETWORK_SRCS = \
    $(SRC_DIR)/network/discovery.c \
    $(SRC_DIR)/network/transport.c

PLATFORM_SRCS = \
    $(SRC_DIR)/platform/cedarx_platform.c

ALL_SRCS = \
    $(CORE_SRCS) \
    $(CONNECTIVITY_SRCS) \
    $(PROTOCOL_SRCS) \
    $(NETWORK_SRCS) \
    $(PLATFORM_SRCS)

ALL_OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(ALL_SRCS:.c=.o)))

# ── Compile flags ─────────────────────────────────────────────────────────

CFLAGS = $(CC_FLAGS) \
    -I$(INCLUDE_DIR) \
    -I$(INCLUDE_DIR)/bsp \
    -fPIC -pthread

# On multi-arch systems the compiler may need its own arch-specific
# system headers. Let the compiler find them automatically by using
# $(shell $(CC) -print-file-name=include) where available; otherwise
# rely on the toolchain's default search paths.
MULTIARCH_INC := $(shell $(CC) -print-multiarch 2>/dev/null)
ifneq ($(MULTIARCH_INC),)
    CFLAGS += -I/usr/include/$(MULTIARCH_INC)
endif

# ── H133 / BSP feature flags ──────────────────────────────────────────────

# Detect sysroot from environment variables
H133_SYSROOT ?= $(SYSROOT)

ifdef H133_SYSROOT
    CFLAGS += -isysroot $(H133_SYSROOT)
    CFLAGS += -DH133_BOARD
    BSP_LIB_DIR = $(H133_SYSROOT)/usr/lib
    BSP_INCLUDE = $(H133_SYSROOT)/usr/include

    CFLAGS += -I$(BSP_INCLUDE)

    BSP_LIBS =
endif

# Feature toggles (passed on command line or auto-detected when H133_SYSROOT set)
ifdef ENABLE_WIFI
    CFLAGS += -DENABLE_WIFI -DLV_USE_LINUX_WIFI2_MODE
    ifdef H133_SYSROOT
        BSP_LIBS += -lwifimg
    endif
endif

ifdef ENABLE_BT
    CFLAGS += -DENABLE_BT -DLV_USE_LINUX_BT4_MODE
    ifdef H133_SYSROOT
        BSP_LIBS += -lbtmanager
    endif
endif

ifdef ENABLE_MIRACAST
    CFLAGS += -DENABLE_MIRACAST
    ifdef H133_SYSROOT
        BSP_LIBS += -lwfd2 -lwpa_client
    endif
endif

ifdef ENABLE_DLNA
    CFLAGS += -DENABLE_DLNA
    ifdef H133_SYSROOT
        BSP_LIBS += -lawdlna
    endif
endif

ifdef ENABLE_AIRPLAY
    CFLAGS += -DENABLE_AIRPLAY
endif

ifdef ENABLE_WIRED
    CFLAGS += -DENABLE_WIRED
endif

# ── Linker flags ──────────────────────────────────────────────────────────

LDFLAGS_SHARED = \
    -shared \
    -Wl,-soname,$(LIB_NAME).so.$(LIB_VERSION_MAJOR) \
    -pthread \
    -ldl

ifdef H133_SYSROOT
    LDFLAGS_SHARED += -L$(BSP_LIB_DIR) $(BSP_LIBS)
endif

# ── Build rules ───────────────────────────────────────────────────────────

.PHONY: all static shared test clean install install-local help

all: static shared

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

# Generic compile rule (searches in all source sub-dirs)
VPATH = \
    $(SRC_DIR)/core \
    $(SRC_DIR)/connectivity \
    $(SRC_DIR)/protocols/miracast \
    $(SRC_DIR)/protocols/airplay \
    $(SRC_DIR)/protocols/dlna \
    $(SRC_DIR)/protocols/wired \
    $(SRC_DIR)/protocols/wireless \
    $(SRC_DIR)/network \
    $(SRC_DIR)/platform

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

-include $(wildcard $(DEP_DIR)/*.d)

static: $(STATIC_LIB)

$(STATIC_LIB): $(ALL_OBJS)
	@echo "  AR  $@"
	@$(AR) rcs $@ $^
	@echo "Built: $@"

shared: $(SHARED_LIB) $(SHARED_LIB_MAJOR) $(SHARED_LIB_BASE)

$(SHARED_LIB): $(ALL_OBJS)
	@echo "  LD  $@"
	@$(CC) $(LDFLAGS_SHARED) -o $@ $^
	@echo "Built: $@"

$(SHARED_LIB_MAJOR): $(SHARED_LIB)
	@ln -sf $(notdir $(SHARED_LIB)) $@

$(SHARED_LIB_BASE): $(SHARED_LIB_MAJOR)
	@ln -sf $(notdir $(SHARED_LIB_MAJOR)) $@

# ── Test / validation app ─────────────────────────────────────────────────

TEST_SRC = examples/basic_example.c
TEST_BIN = $(BUILD_DIR)/screenmirror_test

test: static
	@echo "  CC  $(TEST_SRC)"
	@$(CC) $(CFLAGS) $(TEST_SRC) -L$(BUILD_DIR) -lscreenmirror -lpthread -o $(TEST_BIN)
	@echo "Test binary: $(TEST_BIN)"
	@echo "Running test..."
	@LD_LIBRARY_PATH=$(BUILD_DIR) $(TEST_BIN)

# ── Install ───────────────────────────────────────────────────────────────

DESTDIR ?= /usr/local

install: all
	@echo "Installing to $(DESTDIR)"
	@install -d $(DESTDIR)/lib $(DESTDIR)/include
	@install -m 644 $(STATIC_LIB) $(DESTDIR)/lib/
	@install -m 755 $(SHARED_LIB) $(DESTDIR)/lib/
	@ln -sf $(notdir $(SHARED_LIB)) $(DESTDIR)/lib/$(LIB_NAME).so.$(LIB_VERSION_MAJOR)
	@ln -sf $(LIB_NAME).so.$(LIB_VERSION_MAJOR) $(DESTDIR)/lib/$(LIB_NAME).so
	@install -m 644 $(INCLUDE_DIR)/screenmirror.h $(DESTDIR)/include/
	@echo "Installation complete"

install-local: all
	@$(MAKE) install DESTDIR=./install

# ── Clean ─────────────────────────────────────────────────────────────────

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

# ── Help ──────────────────────────────────────────────────────────────────

help:
	@echo ""
	@echo "libscreenmirror v$(LIB_VERSION_MAJOR).$(LIB_VERSION_MINOR).$(LIB_VERSION_PATCH) Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all            Build static + shared libraries (default)"
	@echo "  static         Build libscreenmirror.a"
	@echo "  shared         Build libscreenmirror.so"
	@echo "  test           Build and run the validation binary"
	@echo "  install        Install to DESTDIR (default: /usr/local)"
	@echo "  install-local  Install to ./install/"
	@echo "  clean          Remove build artifacts"
	@echo ""
	@echo "Host build (stub mode):"
	@echo "  make all"
	@echo ""
	@echo "H133 cross-compile:"
	@echo "  make all \\"
	@echo "       CROSS_COMPILE=arm-linux-gnueabihf- \\"
	@echo "       H133_SYSROOT=/opt/h133-sysroot \\"
	@echo "       ENABLE_WIFI=1 ENABLE_BT=1 \\"
	@echo "       ENABLE_MIRACAST=1 ENABLE_DLNA=1 \\"
	@echo "       ENABLE_AIRPLAY=1 ENABLE_WIRED=1"
	@echo ""
