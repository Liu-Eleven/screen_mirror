TARGET := libwpa_client.so

C_SRCS := cli.c \
        common.c \
        edit.c \
        eloop.c \
        httpread.c \
        http_server.c \
        os_unix.c \
        radiotap.c \
        trace.c \
        wpabuf.c \
        wpa_ctrl.c \
        wpa_debug.c

OBJS := $(C_SRCS:.c=.o)

INCLUDE_PATHS := -I./ \
				 -I./include

LOCAL_CFLAGS := -DCONFIG_CTRL_IFACE_UNIX

all: $(TARGET)

$(TARGET): $(OBJS)
	echo "Linking $@ with $(OBJS)"
	$(CC) -fPIC -shared -o $@ $(OBJS) $(LIB_PATH) $(LIBS)

%.o: %.c
	echo "Compiling $< to $@"
	$(CC) -fPIC $(CFLAGS) $(LOCAL_CFLAGS) $(INCLUDE_PATHS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

# Force serial compilation to avoid file truncation issues
.NOTPARALLEL:
.NO_PARALLEL: $(OBJS) $(TARGET)

