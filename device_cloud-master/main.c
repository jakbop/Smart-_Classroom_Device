#include <stdio.h>
#include <signal.h>
#include "mqtt_client.h"
#include "device_server.h"


int main()
{
	// 忽略 SIGPIPE 信号
	signal(SIGPIPE, SIG_IGN);

    // 连接 MQTT 服务器，订阅感兴趣的主题
    mqttclient_comm_init();

    // 初始化设备服务器，接收设备连接
    device_server_init();

    return 0;
}