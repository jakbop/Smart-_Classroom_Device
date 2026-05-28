/**
 * @file mqtt_client.h
 * @brief MQTT客户端模块接口
 *
 * 声明MQTT客户端的初始化函数和消息发布函数，该模块负责：
 *   - 连接本地Mosquitto MQTT服务器
 *   - 订阅 "+/cmd" 通配主题接收控制命令
 *   - 收到控制命令后通过哈希表查找设备socket并转发
 *   - 提供MQTT消息发布接口供device_server模块调用
 */

#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_


#include "MQTTAsync.h"


/* 全局MQTT客户端句柄，供device_server模块调用mqttclient_pub_msg()时使用 */
extern MQTTAsync g_client_itmojun;


/**
 * @brief 初始化MQTT客户端并建立连接
 *
 * 创建MQTT异步客户端，设置回调函数，配置自动重连，发起连接请求。
 * 连接成功后自动订阅 "+/cmd" 主题。
 */
void mqttclient_comm_init();


/**
 * @brief 发布MQTT消息
 *
 * @param client  MQTT客户端句柄指针
 * @param topic   MQTT主题（如 "wengyuanhang/sensor/Temp"）
 * @param payload 消息载荷（如 "28.22_65"）
 */
void mqttclient_pub_msg(MQTTAsync* client, const char* topic, const char* payload);


#endif
