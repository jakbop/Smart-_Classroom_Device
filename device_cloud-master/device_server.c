#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>

#include "mqtt_client.h"
#include "device_server.h"
#include "hash_table.h"

#define MAX_ONLINE_DEVICE_CNT 10000


typedef struct 
{
	int sock_conn;
	char ip[16];
	unsigned short port;

} client_info;



void* client_communication(void* arg);
int recv_line(int sock, char* buf, int maxlen);


void device_server_init(void)
{
	ci_table = create_hash_table(MAX_ONLINE_DEVICE_CNT);

	// 第 1 步：创建监听套接字
	int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

	if(sock_listen == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// 开启地址复用，以便服务器快速重启
	int opt_val = 1;
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	// 第 2 步：绑定地址
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;                           // 指定地址家族为 Internet 地址家族
	//myaddr.sin_addr.s_addr = inet_addr("47.97.81.90");     // 指定要绑定的 IP 地址为本机的某个 IP 地址
	myaddr.sin_addr.s_addr = INADDR_ANY;                     // 指定要绑定的 IP 地址为本机任意 IP 地址
	myaddr.sin_port = htons(8866); // 指定要绑定的端口号

	if(bind(sock_listen, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	// 第 3 步：监听
	if(listen(sock_listen, 5) == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	// 设置超时时间为10秒
	struct timeval tv;
	tv.tv_sec = 10;     // 多少秒
	tv.tv_usec = 0;     // 多少微秒

	pthread_t tid;
	struct sockaddr_in client_addr;           // 用于接收客户端地址信息
	socklen_t addr_len = sizeof(client_addr); // 用于接收客户端地址信息的长度
	client_info* ci = NULL;
	int sock_conn;


	while(1)
	{
		// 第 4 步：接受客户端连接请求
		sock_conn = accept(sock_listen, (struct sockaddr*)&client_addr, &addr_len);  // 如果对客户端地址感兴趣，后两个参数就不要传 NULL

		if(sock_conn == -1)
		{
			perror("accept");
			continue;
		}

		if (setsockopt(sock_conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) 
		{
			perror("setsockopt");
		}

		printf("\n客户端(%s:%hu)连接成功！\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// 创建新线程，负责与客户端通信
		ci = malloc(sizeof(client_info));

		if(ci == NULL)
		{
			perror("malloc");
			close(sock_conn);
			continue;
		}

		ci->sock_conn = sock_conn;
		strcpy(ci->ip, inet_ntoa(client_addr.sin_addr));
		ci->port = ntohs(client_addr.sin_port);

		if(pthread_create(&tid, NULL, client_communication, ci))
		{
			perror("pthread_create");
			close(sock_conn);
			free(ci);
			continue;
		}
	}

	// 第 7 步：关闭监听套接字
	close(sock_listen);
}



// 通信线程函数
void* client_communication(void* arg)
{
	client_info* ci = arg;
	char dev_id[31];  // 设备 ID 不能超过 30 个字符
	int is_first_online = 1;


	pthread_detach(pthread_self());


	// 第 5 步：收发数据
	char msg[1000];
	int ret;

	while(1)
	{
		//ret = recv(ci->sock_conn, msg, sizeof(msg)-1, 0);
		ret = recv_line(ci->sock_conn, msg, sizeof(msg));

		if(ret <= 0) break;

		printf("\n终端设备说：%s\n", msg);

		char* topic = NULL;
		char* payload = NULL;

		topic = strtok(msg, " ");
		payload = strtok(NULL, " ");

		if(topic != NULL && payload != NULL)
		{
			// 获取设备 ID
			char* p = strchr(topic, '/');
			int len;

			if(p == NULL) break;

			len = p - topic;

			if(len > 30) break;

			if(is_first_online)
			{
				strncpy(dev_id, topic, len);
				dev_id[len] = '\0';

				// 将设备 ID 和对应的连接套接字插入哈希表
				hash_table_insert(ci_table, dev_id, (void*)(long)(ci->sock_conn));
				is_first_online = 0;
			}
			else
			{
				// 判断设备 ID 是否发生变化
				if(strncmp(dev_id, topic, len) != 0) break;
			}

			// 发布消息到 MQTT 服务器
			mqttclient_pub_msg(&g_client_itmojun, topic, payload);
		}

	}


	// 第 6 步：断开连接（关闭连接套接字，后面就不能再和这个连接套接字对应的客户端通信了）
	close(ci->sock_conn);
	printf("\n客户端(%s:%hu)断开连接！\n", ci->ip, ci->port);	

	free(ci);

	// 当设备离线后，将其从哈希表删除
	hash_table_remove(ci_table, dev_id);

	return NULL;
}



int recv_line(int sock, char* buf, int maxlen)
{
	int n, rc;
	char c;
	for(n = 1; n < maxlen; n++)
	{
		if((rc = recv(sock, &c, 1, 0)) == 1)
		{
			if(c == '\n')
				break;

			*buf++ = c;
		}
		else if(rc == 0)
		{
			if(n == 1)
				return 0; // EOF, no data read
			else
				break;    // EOF, some data was read
		}
		else
		{
			return -1;    // error
		}
	}
	*buf = '\0';
	return n;
}
