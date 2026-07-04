#
# libscreenmirror Makefile
#

CC ?= gcc
AR ?= ar
CC_FLAGS ?= -O2 -g -Wall -Wextra -Wno-unused-function
LIB_NAME = libscreenmirror
LIB_VERSION_MAJOR = 1
LIB_VERSION_MINOR = 0
LIB_VERSION_PATCH = 0

# Paths
LIB_DIR = .
INCLUDE_DIR = $(LIB_DIR)/include
SRC_DIR = $(LIB_DIR)/src
BUILD_DIR = $(LIB_DIR)/build
OBJ_DIR = $(BUILD_DIR)/obj
DEP_DIR = $(BUILD_DIR)/dep
ADAPTER_DIR = ../lv_projector/src/ProjectorAdapter
TEST_DIR = $(LIB_DIR)/tests

# Output
STATIC_LIB = $(BUILD_DIR)/$(LIB_NAME).a
SHARED_LIB = $(BUILD_DIR)/$(LIB_NAME).so.$(LIB_VERSION_MAJOR).$(LIB_VERSION_MINOR).$(LIB_VERSION_PATCH)
SHARED_LIB_MAJOR = $(BUILD_DIR)/$(LIB_NAME).so.$(LIB_VERSION_MAJOR)
SHARED_LIB_BASE = $(BUILD_DIR)/$(LIB_NAME).so
ADAPTER_TEST_BIN = $(BUILD_DIR)/projector_adapter_test

# Source files
CORE_SRCS = \
    $(SRC_DIR)/core/mirror_engine.c \
    $(SRC_DIR)/core/event_system.c \
    $(SRC_DIR)/core/state_machine.c

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

ALL_SRCS = $(CORE_SRCS) $(PROTOCOL_SRCS) $(NETWORK_SRCS) $(PLATFORM_SRCS)
ALL_OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(ALL_SRCS:.c=.o)))
ALL_DEPS = $(addprefix $(DEP_DIR)/, $(notdir $(ALL_SRCS:.c=.d)))

# Compile flags
CFLAGS = $(CC_FLAGS) -I$(INCLUDE_DIR) -fPIC -pthread
LDFLAGS_SHARED = -shared -Wl,-soname,$(LIB_NAME).so.$(LIB_VERSION_MAJOR) -pthread

.PHONY: all static shared clean install adapter-test test

# Default target
all: static shared

# Create build directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/core/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/protocols/miracast/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/protocols/airplay/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/protocols/dlna/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/protocols/wired/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/protocols/wireless/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/network/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/platform/%.c | $(OBJ_DIR) $(DEP_DIR)
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Static library
static: $(STATIC_LIB)

$(STATIC_LIB): $(ALL_OBJS)
	@echo "AR $@"
	@$(AR) rcs $@ $^

# Shared library
shared: $(SHARED_LIB) $(SHARED_LIB_MAJOR) $(SHARED_LIB_BASE)

$(SHARED_LIB): $(ALL_OBJS)
	@echo "LD $@"
	@$(CC) $(LDFLAGS_SHARED) -o $@ $^

$(SHARED_LIB_MAJOR): $(SHARED_LIB)
	@ln -sf $(notdir $(SHARED_LIB)) $@

$(SHARED_LIB_BASE): $(SHARED_LIB_MAJOR)
	@ln -sf $(notdir $(SHARED_LIB_MAJOR)) $@

# Install targets
install: all
	@echo "Installing to /usr/local/lib and /usr/local/include"
	@mkdir -p /usr/local/lib /usr/local/include
	@cp $(STATIC_LIB) /usr/local/lib/
	@cp $(SHARED_LIB) /usr/local/lib/
	@cp $(SHARED_LIB_MAJOR) /usr/local/lib/ 2>/dev/null || true
	@cp $(SHARED_LIB_BASE) /usr/local/lib/ 2>/dev/null || true
	@cp $(INCLUDE_DIR)/screenmirror.h /usr/local/include/
	@echo "Installation complete"

install-local: all
	@echo "Installing to ./install directory"
	@mkdir -p ./install/lib ./install/include
	@cp $(STATIC_LIB) ./install/lib/
	@cp $(SHARED_LIB) ./install/lib/
	@cp $(SHARED_LIB_MAJOR) ./install/lib/ 2>/dev/null || true
	@cp $(SHARED_LIB_BASE) ./install/lib/ 2>/dev/null || true
	@cp $(INCLUDE_DIR)/screenmirror.h ./install/include/
	@echo "Local installation complete"

$(ADAPTER_TEST_BIN): $(STATIC_LIB) \
		$(ADAPTER_DIR)/projector_adapter.c \
		$(ADAPTER_DIR)/ui_event_handler.c \
		$(ADAPTER_DIR)/mirror_event_handler.c \
		$(TEST_DIR)/projector_adapter_test.c
	@echo "CC $@"
	@$(CC) $(CFLAGS) \
		-I$(ADAPTER_DIR) \
		-o $@ \
		$(TEST_DIR)/projector_adapter_test.c \
		$(ADAPTER_DIR)/projector_adapter.c \
		$(ADAPTER_DIR)/ui_event_handler.c \
		$(ADAPTER_DIR)/mirror_event_handler.c \
		$(STATIC_LIB)

adapter-test: $(ADAPTER_TEST_BIN)

test: adapter-test
	@$(ADAPTER_TEST_BIN)

# Clean
clean:
	@echo "Cleaning build directory"
	@rm -rf $(BUILD_DIR)

# Help
help:
	@echo "libscreenmirror Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build both static and shared libraries (default)"
	@echo "  static           - Build static library (.a)"
	@echo "  shared           - Build shared library (.so)"
	@echo "  install          - Install to /usr/local"
	@echo "  install-local    - Install to ./install directory"
	@echo "  clean            - Remove build artifacts"
	@echo "  help             - Show this help message"

.PHONY: help
