/**
 * @file device_server.h
 * @brief TCP设备服务器模块接口
 *
 * 声明TCP设备服务器的初始化函数，该函数负责：
 *   - 创建TCP监听套接字（端口8866）
 *   - 接受ESP8266等终端设备的连接
 *   - 为每个连接创建独立通信线程
 *   - 解析设备上报数据并转发到MQTT
 */

#ifndef _DEVICE_SERVER_H_
#define _DEVICE_SERVER_H_


/**
 * @brief 初始化并启动TCP设备服务器
 *
 * 创建哈希表、TCP监听套接字，进入accept循环。
 * 该函数内部是无限循环，正常情况下不会返回。
 */
void device_server_init(void);



#endif //_DEVICE_SERVER_H_
