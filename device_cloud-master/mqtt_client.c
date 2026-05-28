/**
 * @file mqtt_client.c
 * @brief MQTT客户端 - 连接MQTT Broker，订阅控制命令并转发给设备
 *
 * 本模块实现MQTT客户端功能，是设备云主机的"北向接口"：
 *   - 连接本地Mosquitto MQTT服务器（127.0.0.1:1883）
 *   - 订阅 "+/cmd" 通配主题，接收所有设备的控制命令
 *   - 收到控制命令后，通过哈希表查找目标设备的TCP socket并转发
 *   - 提供MQTT消息发布接口，供device_server模块转发传感器数据
 *
 * 数据流向：
 *   上行（设备→云端）：TCP接收 → mqttclient_pub_msg() → MQTT发布 → H5/小智
 *   下行（云端→设备）：H5/小智 → MQTT订阅 → messageArrived() → 哈希表查找 → TCP发送
 *
 * MQTT主题设计：
 *   传感器数据: wengyuanhang/sensor/Temp   (温湿度)
 *               wengyuanhang/sensor/Light  (光照)
 *   设备状态:   wengyuanhang/state/Led1    (LED1状态)
 *               wengyuanhang/state/Led2    (LED2状态)
 *   控制命令:   wengyuanhang/cmd           (控制指令，载荷为a/b/c等)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "MQTTAsync.h"
#include "hash_table.h"

/* MQTT服务器地址，连接本机Mosquitto */
#define ADDRESS_ITMOJUN         "tcp://127.0.0.1:1883"

/* MQTT客户端ID，必须全局唯一，同一ID只能有一个连接 */
#define CLIENTID_ITMOJUN        "junge_yyds_999"

/* MQTT QoS等级
 * 0: 最多一次（可能丢失）
 * 1: 最少一次（不会丢失，可能重复）← 本项目使用
 * 2: 恰好一次（不会丢失，不会重复，性能最低）
 */
#define QOS             1

/* MQTT连接状态标志 */
#define MQTT_CONNECTED    1
#define MQTT_DISCONNECTED 0 



/* 全局MQTT客户端句柄，供device_server模块调用发布消息 */
MQTTAsync g_client_itmojun;

/* MQTT连接状态：CONNECTED 或 DISCONNECTED */
static volatile int g_connected_itmojun = MQTT_DISCONNECTED;



/**
 * @brief MQTT消息发送失败回调
 *
 * 当MQTT消息发送失败时被调用，通常原因：
 *   - 网络中断
 *   - Broker不可达
 *   - QoS 1/2 的确认超时
 *
 * @param context  用户上下文
 * @param response 失败详情，包含token和错误码
 */
static void onSendFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Message send failed token %d error code %d\n", response->token, response->code);
}


/**
 * @brief MQTT消息发送成功回调
 *
 * QoS 1时，Broker确认收到消息后触发此回调
 *
 * @param context  用户上下文
 * @param response 成功详情
 */
static void onSend(void* context, MQTTAsync_successData* response)
{
    /* 消息已确认发送成功，无需特殊处理 */
}
 


/**
 * @brief 发布MQTT消息
 *
 * 将传感器数据或设备状态发布到指定的MQTT主题。
 * 仅在MQTT连接正常时才会发送，否则打印错误信息。
 *
 * @param client  MQTT客户端句柄指针
 * @param topic   MQTT主题（如 "wengyuanhang/sensor/Temp"）
 * @param payload 消息载荷（如 "28.22_65"）
 *
 * @note 使用static局部变量保存消息和选项，避免频繁分配内存
 *       这意味着该函数不是线程安全的，同一时间只能有一个发布操作
 */
void mqttclient_pub_msg(MQTTAsync* client, const char* topic, const char* payload)
{
    static MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    static MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
    int rc;

    int connect_state = g_connected_itmojun;

    if(MQTT_CONNECTED == connect_state)
    {
        /* 设置发布回调 */
        pub_opts.onSuccess = onSend;
        pub_opts.onFailure = onSendFailure;
        pub_opts.context = *client;

        /* 设置消息内容 */
        pubmsg.payload = (void*)payload;
        pubmsg.payloadlen = strlen(payload);  /* 载荷长度（不含'\0'） */
        pubmsg.qos = QOS;                     /* 服务质量等级 */
        pubmsg.retained = 0;                  /* 不保留消息（Broker不存储最后一条） */

        /* 异步发送MQTT消息
         * MQTTAsync_sendMessage()是非阻塞的，发送结果通过回调通知
         */
        if ((rc = MQTTAsync_sendMessage(*client, topic, &pubmsg, &pub_opts)) != MQTTASYNC_SUCCESS)
        {
            fprintf(stderr, "Failed to start sendMessage, return code %d\n", rc);
        }
    }
    else
    {
        fprintf(stderr, "mqtt disconnected...\n");
    }
}


/**
 * @brief MQTT订阅成功回调
 */
static void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
}


/**
 * @brief MQTT订阅失败回调
 */
static void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
}


/**
 * @brief MQTT首次连接成功回调
 *
 * 在MQTTAsync_connect()成功建立TCP连接后触发。
 * 注意：此回调不等同于"可以收发消息"，需要等待onConnectCallCBack才算完全就绪。
 */
static void onConnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful connection\n");

    g_connected_itmojun = MQTT_CONNECTED;
    
}


/**
 * @brief MQTT首次连接失败回调
 */
static void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Connect failed, rc %d\n", response ? response->code : 0);
 
    g_connected_itmojun = MQTT_DISCONNECTED;
}


/**
 * @brief MQTT连接丢失回调
 *
 * 当与Broker的TCP连接意外断开时触发（如网络故障、Broker重启）。
 * 由于配置了automaticReconnect，paho-mqtt库会自动尝试重连。
 *
 * @param context 用户上下文
 * @param cause   连接丢失的原因描述
 */
static void connlost(void *context, char *cause)
{
    printf("\nERROR:Connection lost,Cause: %s,Reconnecting...\n", cause);
 
    g_connected_itmojun = MQTT_DISCONNECTED;
}


/**
 * @brief MQTT连接成功后的自动订阅回调
 *
 * 此回调在以下情况触发：
 *   1. 首次连接成功
 *   2. 断线重连成功
 *
 * 在此回调中订阅 "+/cmd" 主题，确保每次连接后都能收到控制命令。
 * "+" 是MQTT单层通配符，匹配任意设备ID，例如：
 *   "wengyuanhang/cmd" ✅ 匹配
 *   "device002/cmd"    ✅ 匹配
 *   "a/b/cmd"          ❌ 不匹配（多层不匹配）
 *
 * @param context MQTT客户端句柄
 * @param cause   连接原因描述
 */
static void onConnectCallCBack(void *context, char *cause)
{
    printf("Successful onConnectCallCBack\n");

    g_connected_itmojun = MQTT_CONNECTED;


	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = context;

    /* 订阅控制命令主题 "+/cmd"
     * 当H5页面或小智AI发布控制命令到 "设备ID/cmd" 时，
     * 此订阅会匹配到，触发messageArrived()回调
     */
	if ((rc = MQTTAsync_subscribe(context, "+/cmd", QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
	}	
}


/**
 * @brief MQTT断开连接失败回调
 */
static void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Disconnect failed\n");
 
}


/**
 * @brief MQTT断开连接成功回调
 */
static void onDisconnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful disconnection\n");
 
}
 



/**
 * @brief MQTT消息到达回调 - 控制命令转发的核心
 *
 * 当收到订阅主题的消息时触发。这是下行数据流的关键节点：
 *   H5/小智 → MQTT Broker → 本回调 → 哈希表查找 → TCP转发 → ESP8266 → STM32
 *
 * 处理流程：
 *   1. 从主题名中提取设备ID（如 "wengyuanhang/cmd" → "wengyuanhang"）
 *   2. 在哈希表中查找该设备ID对应的TCP socket
 *   3. 如果设备在线（socket > 0），将控制命令载荷通过TCP发送给设备
 *   4. 如果设备不在线，打印提示（命令被丢弃）
 *
 * @param context  用户上下文
 * @param topicName MQTT主题名（如 "wengyuanhang/cmd"）
 * @param topicLen  主题名长度
 * @param message   MQTT消息结构体，包含载荷和属性
 * @return 1 表示消息处理完成
 *
 * @note 控制命令载荷示例：
 *       "a" → 开灯（LED1=SET, LED2=RESET）
 *       "b" → 关灯（LED1=RESET, LED2=SET）
 *       "c" → 开蜂鸣器
 *       "d" → 关蜂鸣器
 */
static int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
    printf("Message arrived\n");
    printf("topic: %s\n", topicName);
    printf("message(%d): %s\n", message->payloadlen, (char*)message->payload);

    /* 从主题中提取设备ID
     * 主题格式: "设备ID/cmd"
     * 例如: "wengyuanhang/cmd" → 设备ID = "wengyuanhang"
     */
    char* p = strchr(topicName, '/');
    char dev_id[31] = "";
    int len;
    int sock_conn;

    if(p != NULL)
    {
	    len = p - topicName;  /* 设备ID的长度 */

	    if(len <= 30)
	    {
		    strncpy(dev_id, topicName, len);  /* 提取设备ID */

		    /* 查询哈希表，获取该设备ID对应的TCP socket描述符
		     * 哈希表中的值是void*类型，存储时将int强转为long再转为void*
		     * 取出时需要反向转换：void* → long → int
		     */
		    sock_conn = (long)hash_table_get(ci_table, dev_id);

		    if(sock_conn > 0)
		    {
			    /* 设备在线，通过TCP socket转发控制命令给ESP8266
			     * ESP8266收到后通过UART转发给STM32执行
			     */
			    send(sock_conn,  (char*)message->payload, message->payloadlen, 0);
			    printf("Forwarded cmd to device %s (socket=%d): %.*s\n", 
			           dev_id, sock_conn, message->payloadlen, (char*)message->payload);
		    }
		    else
		    {
			    /* 设备不在线，控制命令被丢弃 */
			    printf("Device %s not online, cmd dropped\n", dev_id);
		    }
	    }
    }


    /* 释放MQTT消息和主题名的内存（paho-mqtt要求必须调用） */
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;  /* 返回1表示消息已成功处理 */
}
 

/**
 * @brief 初始化MQTT客户端并建立连接
 *
 * 完成以下工作：
 *   1. 创建MQTT异步客户端对象
 *   2. 设置回调函数（连接丢失、消息到达、连接成功自动订阅）
 *   3. 配置连接参数（KeepAlive、CleanSession、自动重连）
 *   4. 发起异步连接请求
 *
 * 连接参数说明：
 *   - keepAliveInterval=20: 每20秒发送一次心跳包，超过1.5倍时间未收到响应则断开
 *   - cleansession=1: 清除会话，每次连接不恢复之前的订阅和消息
 *   - automaticReconnect=1: 启用自动重连
 *   - minRetryInterval=1: 最小重连间隔1秒
 *   - maxRetryInterval=5: 最大重连间隔5秒（指数退避）
 *
 * @note 自动重连成功后会触发onConnectCallCBack，自动重新订阅 "+/cmd"
 */
void mqttclient_comm_init()
{
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    /* 创建MQTT异步客户端对象
     * 参数：客户端句柄、服务器地址、客户端ID、持久化方式、上下文
     * MQTTCLIENT_PERSISTENCE_NONE: 不持久化消息（内存模式）
     */
	if ((rc = MQTTAsync_create(&g_client_itmojun, ADDRESS_ITMOJUN, CLIENTID_ITMOJUN, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}


    /* 设置MQTT回调函数
     * connlost:       连接丢失时触发
     * messageArrived: 收到订阅消息时触发
     * NULL:           交付完成回调（QoS 1/2消息确认送达），不使用
     */
    if ((rc = MQTTAsync_setCallbacks(g_client_itmojun, g_client_itmojun, connlost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to set callback, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    /* 设置连接成功后的自动订阅回调
     * 此回调在首次连接和每次重连成功后都会触发
     * 确保每次连接后都自动订阅 "+/cmd" 主题
     */
    if ((rc = MQTTAsync_setConnected(g_client_itmojun, g_client_itmojun, onConnectCallCBack)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to set callback, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    /* 配置连接选项 */
    conn_opts.keepAliveInterval = 20;    /* 心跳间隔20秒 */
    conn_opts.cleansession = 1;          /* 清除会话模式，不恢复之前的订阅状态 */
    conn_opts.onSuccess = onConnect;     /* 首次连接成功回调 */
    conn_opts.onFailure = onConnectFailure;  /* 首次连接失败回调 */
    conn_opts.context = g_client_itmojun;    /* 传递给回调的上下文 */

    /* 自动重连配置
     * 启用后，连接丢失时paho-mqtt库会自动尝试重连，无需手动处理
     * 重连间隔采用指数退避策略：1s → 2s → 4s → 5s → 5s...
     * 重连成功后会触发onConnectCallCBack，自动重新订阅主题
     */
    conn_opts.automaticReconnect = 1;    /* 启用自动重连 */
    conn_opts.minRetryInterval = 1;      /* 最小重连间隔1秒 */
    conn_opts.maxRetryInterval = 5;      /* 最大重连间隔5秒 */

    /* 发起异步连接请求
     * 连接结果通过回调通知，不会阻塞
     */
    if ((rc = MQTTAsync_connect(g_client_itmojun, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to start connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}
