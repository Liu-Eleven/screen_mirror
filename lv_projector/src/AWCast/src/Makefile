# Lock file for serial compilation
LOCK_FILE := .build_lock

# Acquire lock
acquire_lock:
	@while [ -f "$(LOCK_FILE)" ]; do \
		echo "Waiting for build lock..."; \
		sleep 1; \
done
	@touch "$(LOCK_FILE)"

# Release lock
release_lock:
	@rm -f "$(LOCK_FILE)"

# Define the library targets
libwfd_player.so:
	@$(MAKE) acquire_lock
	echo "$(MAKE) -C wfd_player ARCH=$(ARCH) AR=$(AR) CC=$(CC) CPP=$(CPP)"
	$(MAKE) -j1 -C wfd_player \
		ARCH="$(ARCH)" \
		AR="$(AR)" \
		CC="$(CC)" \
		CPP="$(CXX)"
	@$(MAKE) release_lock

libwpa_client.so:
	@$(MAKE) acquire_lock
	echo "$(MAKE) -C wpa_client ARCH=$(ARCH) AR=$(AR) CC=$(CC) CPP=$(CPP)"
	$(MAKE) -j1 -C wpa_client \
		ARCH="$(ARCH)" \
		AR="$(AR)" \
		CC="$(CC)" \
		CPP="$(CXX)"
	@$(MAKE) release_lock

libwfd2.so:
	@$(MAKE) acquire_lock
	echo "$(MAKE) -C libwfd2 ARCH=$(ARCH) AR=$(AR) CC=$(CC) CPP=$(CPP)"
	$(MAKE) -j1 -C libwfd2 \
		ARCH="$(ARCH)" \
		AR="$(AR)" \
		CC="$(CC)" \
		CPP="$(CXX)"
	@$(MAKE) release_lock

# Default target
all:
	@$(MAKE) libwfd_player.so
	@$(MAKE) libwpa_client.so
	@$(MAKE) libwfd2.so

# Clean target
clean:
	$(MAKE) -C wfd_player clean
	$(MAKE) -C wpa_client clean
	$(MAKE) -C libwfd2 clean
	@rm -f "$(LOCK_FILE)"

# Phony targets
.PHONY: all clean acquire_lock release_lock

# Force serial compilation
.NOTPARALLEL:
