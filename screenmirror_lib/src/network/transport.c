#include "network/transport.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* 网络传输实现 */

/* TCP 传输结构 */
typedef struct {
    int sock;
    char host[256];
    int port;
    bool connected;
} TcpTransport;

/* UDP 传输结构 */
typedef struct {
    int sock;
    char host[256];
    int port;
    struct sockaddr_in addr;
    bool connected;
} UdpTransport;

/* USB 传输结构 */
typedef struct {
    int usb_fd;
    char device_path[256];
    bool connected;
} UsbTransport;

/**
 * 创建 TCP 传输
 */
static TransportHandle tcp_create(const char *host, int port)
{
    TcpTransport *trans = (TcpTransport *)malloc(sizeof(TcpTransport));
    if (trans == NULL) {
        return NULL;
    }
    
    memset(trans, 0, sizeof(TcpTransport));
    strncpy(trans->host, host, sizeof(trans->host) - 1);
    trans->port = port;
    trans->sock = -1;
    trans->connected = false;
    
    printf("[TRANSPORT] TCP created (host: %s, port: %d)\n", host, port);
    
    return (TransportHandle)trans;
}

/**
 * 销毁 TCP 传输
 */
static void tcp_destroy(TransportHandle handle)
{
    TcpTransport *trans = (TcpTransport *)handle;
    if (trans == NULL) {
        return;
    }
    
    if (trans->sock >= 0) {
        close(trans->sock);
    }
    
    free(trans);
    printf("[TRANSPORT] TCP destroyed\n");
}

/**
 * TCP 发送数据
 */
static int tcp_send(TransportHandle handle, const uint8_t *data, int size)
{
    TcpTransport *trans = (TcpTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    int sent = send(trans->sock, data, size, 0);
    if (sent < 0) {
        printf("[TRANSPORT] TCP send failed: %s\n", strerror(errno));
    }
    
    return sent;
}

/**
 * TCP 接收数据
 */
static int tcp_recv(TransportHandle handle, uint8_t *buffer, int size)
{
    TcpTransport *trans = (TcpTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    int received = recv(trans->sock, buffer, size, 0);
    if (received < 0) {
        printf("[TRANSPORT] TCP recv failed: %s\n", strerror(errno));
    }
    
    return received;
}

/**
 * TCP 连接检查
 */
static bool tcp_is_connected(TransportHandle handle)
{
    TcpTransport *trans = (TcpTransport *)handle;
    if (trans == NULL) {
        return false;
    }
    return trans->connected;
}

/**
 * 创建 UDP 传输
 */
static TransportHandle udp_create(const char *host, int port)
{
    UdpTransport *trans = (UdpTransport *)malloc(sizeof(UdpTransport));
    if (trans == NULL) {
        return NULL;
    }
    
    memset(trans, 0, sizeof(UdpTransport));
    strncpy(trans->host, host, sizeof(trans->host) - 1);
    trans->port = port;
    
    trans->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (trans->sock < 0) {
        printf("[TRANSPORT] Failed to create UDP socket\n");
        free(trans);
        return NULL;
    }
    
    memset(&trans->addr, 0, sizeof(trans->addr));
    trans->addr.sin_family = AF_INET;
    trans->addr.sin_port = htons(port);
    inet_aton(host, &trans->addr.sin_addr);
    
    trans->connected = true;
    
    printf("[TRANSPORT] UDP created (host: %s, port: %d)\n", host, port);
    
    return (TransportHandle)trans;
}

/**
 * 销毁 UDP 传输
 */
static void udp_destroy(TransportHandle handle)
{
    UdpTransport *trans = (UdpTransport *)handle;
    if (trans == NULL) {
        return;
    }
    
    if (trans->sock >= 0) {
        close(trans->sock);
    }
    
    free(trans);
    printf("[TRANSPORT] UDP destroyed\n");
}

/**
 * UDP 发送数据
 */
static int udp_send(TransportHandle handle, const uint8_t *data, int size)
{
    UdpTransport *trans = (UdpTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    int sent = sendto(trans->sock, data, size, 0,
                     (struct sockaddr *)&trans->addr, sizeof(trans->addr));
    if (sent < 0) {
        printf("[TRANSPORT] UDP send failed: %s\n", strerror(errno));
    }
    
    return sent;
}

/**
 * UDP 接收数据
 */
static int udp_recv(TransportHandle handle, uint8_t *buffer, int size)
{
    UdpTransport *trans = (UdpTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int received = recvfrom(trans->sock, buffer, size, 0,
                           (struct sockaddr *)&addr, &addr_len);
    if (received < 0) {
        printf("[TRANSPORT] UDP recv failed: %s\n", strerror(errno));
    }
    
    return received;
}

/**
 * UDP 连接检查
 */
static bool udp_is_connected(TransportHandle handle)
{
    UdpTransport *trans = (UdpTransport *)handle;
    if (trans == NULL) {
        return false;
    }
    return trans->connected;
}

/**
 * 创建 USB 传输
 */
static TransportHandle usb_create(const char *device_path, int port)
{
    UsbTransport *trans = (UsbTransport *)malloc(sizeof(UsbTransport));
    if (trans == NULL) {
        return NULL;
    }
    
    memset(trans, 0, sizeof(UsbTransport));
    strncpy(trans->device_path, device_path, sizeof(trans->device_path) - 1);
    trans->usb_fd = -1;
    trans->connected = false;
    
    printf("[TRANSPORT] USB created (device: %s)\n", device_path);
    
    return (TransportHandle)trans;
}

/**
 * 销毁 USB 传输
 */
static void usb_destroy(TransportHandle handle)
{
    UsbTransport *trans = (UsbTransport *)handle;
    if (trans == NULL) {
        return;
    }
    
    if (trans->usb_fd >= 0) {
        close(trans->usb_fd);
    }
    
    free(trans);
    printf("[TRANSPORT] USB destroyed\n");
}

/**
 * USB 发送数据
 */
static int usb_send(TransportHandle handle, const uint8_t *data, int size)
{
    UsbTransport *trans = (UsbTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    int sent = write(trans->usb_fd, data, size);
    if (sent < 0) {
        printf("[TRANSPORT] USB send failed: %s\n", strerror(errno));
    }
    
    return sent;
}

/**
 * USB 接收数据
 */
static int usb_recv(TransportHandle handle, uint8_t *buffer, int size)
{
    UsbTransport *trans = (UsbTransport *)handle;
    if (trans == NULL || !trans->connected) {
        return -1;
    }
    
    int received = read(trans->usb_fd, buffer, size);
    if (received < 0) {
        printf("[TRANSPORT] USB recv failed: %s\n", strerror(errno));
    }
    
    return received;
}

/**
 * USB 连接检查
 */
static bool usb_is_connected(TransportHandle handle)
{
    UsbTransport *trans = (UsbTransport *)handle;
    if (trans == NULL) {
        return false;
    }
    return trans->connected;
}

/* 公开 API 实现 */

/**
 * 创建传输通道
 */
TransportHandle transport_create(TransportType type, const char *host, int port)
{
    if (host == NULL) {
        return NULL;
    }
    
    switch (type) {
        case TRANSPORT_TYPE_TCP:
            return tcp_create(host, port);
        case TRANSPORT_TYPE_UDP:
            return udp_create(host, port);
        case TRANSPORT_TYPE_USB:
            return usb_create(host, port);
        default:
            return NULL;
    }
}

/**
 * 销毁传输通道
 */
void transport_destroy(TransportHandle handle)
{
    if (handle == NULL) {
        return;
    }
    
    /* 这里需要区分类型，简单起见假设都用 TCP 销毁 */
    TcpTransport *trans = (TcpTransport *)handle;
    
    /* 尝试作为 TCP 销毁 */
    tcp_destroy(handle);
}

/**
 * 发送数据
 */
int transport_send(TransportHandle handle, const uint8_t *data, int size)
{
    if (handle == NULL || data == NULL || size <= 0) {
        return -1;
    }
    
    /* 这里简单起见假设都是 TCP */
    return tcp_send(handle, data, size);
}

/**
 * 接收数据
 */
int transport_recv(TransportHandle handle, uint8_t *buffer, int size)
{
    if (handle == NULL || buffer == NULL || size <= 0) {
        return -1;
    }
    
    /* 这里简单起见假设都是 TCP */
    return tcp_recv(handle, buffer, size);
}

/**
 * 检查连接状态
 */
bool transport_is_connected(TransportHandle handle)
{
    if (handle == NULL) {
        return false;
    }
    
    /* 这里简单起见假设都是 TCP */
    return tcp_is_connected(handle);
}
