/**
 * @file main.c
 * @brief 智能物联网设备云主机 - 程序入口
 *
 * 本程序是物联网系统的核心网关，部署在阿里云ECS服务器上，具有双重身份：
 *   1. TCP Server（端口8866）：接收ESP8266/STM32终端设备上报的传感器数据
 *   2. MQTT Client：将传感器数据转发到MQTT Broker，同时订阅控制命令主题
 *
 * 系统架构：
 *   STM32 ←UART→ ESP8266 ←TCP:8866→ 本网关 ←MQTT→ Mosquitto ←MQTT→ H5/小智AI
 *
 * 启动流程：
 *   1. 忽略SIGPIPE信号（防止对端断开导致进程崩溃）
 *   2. 初始化MQTT客户端，连接本地Mosquitto并订阅 +/cmd 主题
 *   3. 初始化TCP服务器，监听8866端口等待设备连接
 */

#include <stdio.h>
#include <signal.h>
#include "mqtt_client.h"
#include "device_server.h"


int main()
{
	/* 忽略 SIGPIPE 信号
	 * 当TCP连接的对端意外关闭时，本端继续调用send()会产生SIGPIPE信号，
	 * 默认行为是终止进程。忽略该信号后，send()会返回-1并设置errno=EPIPE，
	 * 程序可以在代码中优雅地处理连接断开的情况，而不是直接崩溃。
	 */
	signal(SIGPIPE, SIG_IGN);

    /* 初始化MQTT客户端
     * - 连接到本地Mosquitto服务器（127.0.0.1:1883）
     * - 设置连接成功回调：订阅 "+/cmd" 通配主题（匹配所有设备的控制命令）
     * - 设置消息到达回调：收到控制命令后，通过哈希表查找设备socket并转发
     * - 配置自动重连（1~5秒指数退避）
     */
    mqttclient_comm_init();

    /* 初始化TCP设备服务器
     * - 创建哈希表（最大容量10000台设备）
     * - 创建TCP监听套接字，绑定0.0.0.0:8866
     * - 进入accept循环，每来一个设备连接就创建新线程处理通信
     * - 该函数内部是死循环，不会返回
     */
    device_server_init();

    return 0;
}
