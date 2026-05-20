#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "MQTTAsync.h"
#include "hash_table.h"

#define ADDRESS_ITMOJUN         "tcp://127.0.0.1:1883"
#define CLIENTID_ITMOJUN        "junge_yyds_999"


#define QOS             1
#define MQTT_CONNECTED 1
#define MQTT_DISCONNECTED 0 



MQTTAsync g_client_itmojun;
static volatile int g_connected_itmojun = MQTT_DISCONNECTED;



static void onSendFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Message send failed token %d error code %d\n", response->token, response->code);
}


static void onSend(void* context, MQTTAsync_successData* response)
{
    // This gets called when a message is acknowledged successfully.
}
 


void mqttclient_pub_msg(MQTTAsync* client, const char* topic, const char* payload)
{
    static MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    static MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
    int rc;

    int connect_state = g_connected_itmojun;

    if(MQTT_CONNECTED == connect_state)
    {
        pub_opts.onSuccess = onSend;
        pub_opts.onFailure = onSendFailure;
        pub_opts.context = *client;

        pubmsg.payload = (void*)payload;
        pubmsg.payloadlen = strlen(payload);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;

        if ((rc = MQTTAsync_sendMessage(*client, topic, &pubmsg, &pub_opts)) != MQTTASYNC_SUCCESS)
        {
            fprintf(stderr, "Failed to start sendMessage, return code %d\n", rc);
            //g_connected = MQTT_DISCONNECTED;
        }
    }
    else
    {
        fprintf(stderr, "mqtt disconnected...\n");
    }
}



static void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	//subscribed = 1;
}


static void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
}


static void onConnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful connection\n");

    g_connected_itmojun = MQTT_CONNECTED;
    
}


static void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Connect failed, rc %d\n", response ? response->code : 0);
 
    g_connected_itmojun = MQTT_DISCONNECTED;
}


static void connlost(void *context, char *cause)
{
    printf("\nERROR:Connection lost,Cause: %s,Reconnecting...\n", cause);
 
    g_connected_itmojun = MQTT_DISCONNECTED;
}


static void onConnectCallCBack(void *context, char *cause)
{
    printf("Successful onConnectCallCBack\n");

    g_connected_itmojun = MQTT_CONNECTED;


	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = context;

    // 订阅控制命令主题，从而可以接收到来自移动端的控制命令
	if ((rc = MQTTAsync_subscribe(context, "+/cmd", QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
	}	
}


static void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
    printf("Disconnect failed\n");
 
}


static void onDisconnect(void* context, MQTTAsync_successData* response)
{
    printf("Successful disconnection\n");
 
}
 



static int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
    printf("Message arrived\n");
    printf("topic: %s\n", topicName);
    printf("message(%d): %s\n", message->payloadlen, (char*)message->payload);

    // 将接收到的消息转发给对应的 ESP8266
    char* p = strchr(topicName, '/');
    char dev_id[31] = "";
    int len;
    int sock_conn;

    if(p != NULL)
    {
	    len = p - topicName;

	    if(len <= 30)
	    {
		    strncpy(dev_id, topicName, len);

		    // 查询哈希表，获取当前设备 ID 对应的连接套接字，并将控制指令发送给对应的设备
		    sock_conn = (long)hash_table_get(ci_table, dev_id);

		    if(sock_conn > 0)
		    {
			    // 如果目标设备在线
			    send(sock_conn,  (char*)message->payload, message->payloadlen, 0);
		    }
	    }
    }


    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}
 

void mqttclient_comm_init()
{
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

	if ((rc = MQTTAsync_create(&g_client_itmojun, ADDRESS_ITMOJUN, CLIENTID_ITMOJUN, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		fprintf(stderr, "Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}


    //设置连接丢失回调
    if ((rc = MQTTAsync_setCallbacks(g_client_itmojun, g_client_itmojun, connlost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to set callback, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    //设置连接成功回调
    if ((rc = MQTTAsync_setConnected(g_client_itmojun, g_client_itmojun, onConnectCallCBack)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to set callback, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = g_client_itmojun;
    //断开重连设置
    conn_opts.automaticReconnect = 1;//设置非零，断开自动重连
    conn_opts.minRetryInterval = 1; //单位秒，重连间隔次数，每次重新连接失败时，重试间隔都会加倍，直到最大间隔
    conn_opts.maxRetryInterval = 5;//单位秒，最大重连尝试间隔

    if ((rc = MQTTAsync_connect(g_client_itmojun, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        fprintf(stderr, "Failed to start connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}


