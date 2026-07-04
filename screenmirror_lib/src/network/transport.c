#include "network/transport.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

typedef struct {
    TransportType type;
    int fd;
    bool connected;
    struct sockaddr_in addr;
    char endpoint[256];
    int port;
} TransportContext;

static TransportHandle create_tcp(const char *host, int port)
{
    TransportContext *ctx = (TransportContext *)calloc(1, sizeof(TransportContext));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->type = TRANSPORT_TYPE_TCP;
    ctx->fd = socket(AF_INET, SOCK_STREAM, 0);
    ctx->port = port;
    snprintf(ctx->endpoint, sizeof(ctx->endpoint), "%s", host);

    if (ctx->fd < 0) {
        free(ctx);
        return NULL;
    }

    memset(&ctx->addr, 0, sizeof(ctx->addr));
    ctx->addr.sin_family = AF_INET;
    ctx->addr.sin_port = htons((uint16_t)port);
    if (inet_aton(host, &ctx->addr.sin_addr) == 0) {
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->connected = (connect(ctx->fd, (struct sockaddr *)&ctx->addr, sizeof(ctx->addr)) == 0);

    return (TransportHandle)ctx;
}

static TransportHandle create_udp(const char *host, int port)
{
    TransportContext *ctx = (TransportContext *)calloc(1, sizeof(TransportContext));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->type = TRANSPORT_TYPE_UDP;
    ctx->fd = socket(AF_INET, SOCK_DGRAM, 0);
    ctx->port = port;
    snprintf(ctx->endpoint, sizeof(ctx->endpoint), "%s", host);
    if (ctx->fd < 0) {
        free(ctx);
        return NULL;
    }

    memset(&ctx->addr, 0, sizeof(ctx->addr));
    ctx->addr.sin_family = AF_INET;
    ctx->addr.sin_port = htons((uint16_t)port);
    if (inet_aton(host, &ctx->addr.sin_addr) == 0) {
        close(ctx->fd);
        free(ctx);
        return NULL;
    }

    ctx->connected = true;
    return (TransportHandle)ctx;
}

static TransportHandle create_usb(const char *path)
{
    TransportContext *ctx = (TransportContext *)calloc(1, sizeof(TransportContext));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->type = TRANSPORT_TYPE_USB;
    ctx->port = 0;
    snprintf(ctx->endpoint, sizeof(ctx->endpoint), "%s", path);

    ctx->fd = open(path, O_RDWR | O_NONBLOCK);
    if (ctx->fd < 0) {
        ctx->connected = false;
        return (TransportHandle)ctx;
    }

    ctx->connected = true;
    return (TransportHandle)ctx;
}

TransportHandle transport_create(TransportType type, const char *host, int port)
{
    if (host == NULL) {
        return NULL;
    }

    switch (type) {
        case TRANSPORT_TYPE_TCP:
            return create_tcp(host, port);
        case TRANSPORT_TYPE_UDP:
            return create_udp(host, port);
        case TRANSPORT_TYPE_USB:
            return create_usb(host);
        default:
            return NULL;
    }
}

void transport_destroy(TransportHandle handle)
{
    TransportContext *ctx = (TransportContext *)handle;
    if (ctx == NULL) {
        return;
    }

    if (ctx->fd >= 0) {
        close(ctx->fd);
    }

    free(ctx);
}

int transport_send(TransportHandle handle, const uint8_t *data, int size)
{
    TransportContext *ctx = (TransportContext *)handle;
    if (ctx == NULL || data == NULL || size <= 0 || !ctx->connected) {
        return -1;
    }

    if (ctx->type == TRANSPORT_TYPE_UDP) {
        return (int)sendto(ctx->fd, data, (size_t)size, 0,
                           (struct sockaddr *)&ctx->addr, sizeof(ctx->addr));
    }

    return (int)write(ctx->fd, data, (size_t)size);
}

int transport_recv(TransportHandle handle, uint8_t *buffer, int size)
{
    TransportContext *ctx = (TransportContext *)handle;
    if (ctx == NULL || buffer == NULL || size <= 0 || !ctx->connected) {
        return -1;
    }

    if (ctx->type == TRANSPORT_TYPE_UDP) {
        struct sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        return (int)recvfrom(ctx->fd, buffer, (size_t)size, 0,
                             (struct sockaddr *)&addr, &addr_len);
    }

    return (int)read(ctx->fd, buffer, (size_t)size);
}

bool transport_is_connected(TransportHandle handle)
{
    TransportContext *ctx = (TransportContext *)handle;
    if (ctx == NULL) {
        return false;
    }
    return ctx->connected;
}
