#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include <stddef.h>

/* 网络传输接口 */

/* 传输类型 */
typedef enum {
    TRANSPORT_TYPE_TCP,
    TRANSPORT_TYPE_UDP,
    TRANSPORT_TYPE_USB,
} TransportType;

/* 传输句柄 */
typedef void* TransportHandle;

/**
 * 创建传输通道
 * @param type: 传输类型
 * @param host: 目标主机IP或设备路径
 * @param port: 目标端口（USB不需要）
 * @return: 传输句柄
 */
TransportHandle transport_create(TransportType type, const char *host, int port);

/**
 * 销毁传输通道
 */
void transport_destroy(TransportHandle handle);

/**
 * 发送数据
 * @param handle: 传输句柄
 * @param data: 数据指针
 * @param size: 数据大小
 * @return: 已发送的字节数，< 0 表示失败
 */
int transport_send(TransportHandle handle, const uint8_t *data, int size);

/**
 * 接收数据
 * @param handle: 传输句柄
 * @param buffer: 接收缓冲区
 * @param size: 缓冲区大小
 * @return: 已接收的字节数，0 表示超时，< 0 表示失败
 */
int transport_recv(TransportHandle handle, uint8_t *buffer, int size);

/**
 * 检查连接状态
 */
bool transport_is_connected(TransportHandle handle);

#endif /* TRANSPORT_H */
