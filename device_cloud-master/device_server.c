/**
 * @file device_server.c
 * @brief TCP设备服务器 - 接收终端设备连接并处理通信
 *
 * 本模块实现TCP服务器功能，是设备云主机的"南向接口"：
 *   - 监听TCP 8866端口，等待ESP8266等终端设备连接
 *   - 每个设备连接分配一个独立线程进行通信
 *   - 按行读取设备上报的数据，解析自定义协议格式
 *   - 将解析后的数据通过MQTT发布到对应主题
 *   - 使用哈希表管理在线设备（设备ID → socket映射）
 *
 * 自定义TCP协议格式：
 *   "主题 载荷\n"
 *   例如："wengyuanhang/sensor/Temp 28.22_65\n"
 *         "wengyuanhang/state/Led1 1\n"
 *
 * 协议解析规则：
 *   - 以空格分隔主题和载荷
 *   - 主题中第一个'/'之前的部分为设备ID
 *   - 以'\n'作为一行数据的结束标志
 */

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

/* 最大在线设备数量，同时也是哈希表的容量 */
#define MAX_ONLINE_DEVICE_CNT 10000


/**
 * @brief 客户端连接信息结构体
 * 用于在新线程创建时传递客户端的连接信息
 */
typedef struct 
{
	int sock_conn;       /* 与客户端通信的套接字描述符 */
	char ip[16];         /* 客户端IP地址（点分十进制） */
	unsigned short port; /* 客户端端口号 */
} client_info;



/* 函数前置声明 */
void* client_communication(void* arg);
int recv_line(int sock, char* buf, int maxlen);


/**
 * @brief 初始化并启动TCP设备服务器
 *
 * 执行流程：
 *   1. 创建哈希表用于管理在线设备
 *   2. 创建TCP监听套接字
 *   3. 设置SO_REUSEADDR允许服务器快速重启
 *   4. 绑定0.0.0.0:8866地址
 *   5. 开始监听（backlog=5）
 *   6. 进入accept循环，每来一个连接创建新线程处理
 *
 * @note 该函数内部是无限循环，正常情况下不会返回
 */
void device_server_init(void)
{
	/* 创建全局哈希表，用于存储 设备ID→socket 的映射关系 */
	ci_table = create_hash_table(MAX_ONLINE_DEVICE_CNT);

	/* 第1步：创建TCP监听套接字
	 * AF_INET: IPv4协议族
	 * SOCK_STREAM: 面向连接的TCP协议
	 * 0: 自动选择协议（TCP）
	 */
	int sock_listen = socket(AF_INET, SOCK_STREAM, 0);

	if(sock_listen == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}

	/* 开启地址复用选项
	 * 服务器关闭后，TCP连接会进入TIME_WAIT状态，默认需要等待2MSL时间才能重新绑定同一端口。
	 * 设置SO_REUSEADDR后，允许立即重新绑定处于TIME_WAIT状态的端口，方便服务器快速重启。
	 */
	int opt_val = 1;
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	/* 第2步：绑定地址
	 * INADDR_ANY: 绑定本机所有可用IP地址（多网卡环境下都能接收连接）
	 * htons(8866): 将端口号8866从主机字节序转换为网络字节序
	 */
	struct sockaddr_in myaddr;
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	myaddr.sin_port = htons(8866);

	if(bind(sock_listen, (struct sockaddr*)&myaddr, sizeof(myaddr)) == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	/* 第3步：开始监听
	 * backlog=5: 内核维护的已完成三次握手但尚未被accept()取走的连接队列最大长度
	 */
	if(listen(sock_listen, 5) == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	/* 设置TCP接收超时时间
	 * 每个设备连接都会设置此超时，防止设备异常断开后线程永久阻塞在recv()上
	 */
	struct timeval tv;
	tv.tv_sec = 10;     /* 超时秒数 */
	tv.tv_usec = 0;     /* 超时微秒数 */

	pthread_t tid;
	struct sockaddr_in client_addr;           /* 用于接收客户端地址信息 */
	socklen_t addr_len = sizeof(client_addr); /* 客户端地址结构体长度 */
	client_info* ci = NULL;
	int sock_conn;


	/* 第4步：主循环 - 接受客户端连接请求
	 * accept()会阻塞等待新的TCP连接到来
	 * 每接受一个连接，就创建一个新线程负责与该客户端通信
	 */
	while(1)
	{
		sock_conn = accept(sock_listen, (struct sockaddr*)&client_addr, &addr_len);

		if(sock_conn == -1)
		{
			perror("accept");
			continue;  /* accept失败不退出，继续等待下一个连接 */
		}

		/* 为新连接设置接收超时
		 * 如果10秒内没有收到数据，recv()会返回-1并设置errno=EAGAIN
		 * 这样可以防止设备异常断开后线程永久阻塞
		 */
		if (setsockopt(sock_conn, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) 
		{
			perror("setsockopt");
		}

		printf("\n客户端(%s:%hu)连接成功！\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		/* 分配客户端信息结构体，用于传递给新线程 */
		ci = malloc(sizeof(client_info));

		if(ci == NULL)
		{
			perror("malloc");
			close(sock_conn);
			continue;
		}

		/* 保存客户端连接信息 */
		ci->sock_conn = sock_conn;
		strcpy(ci->ip, inet_ntoa(client_addr.sin_addr));
		ci->port = ntohs(client_addr.sin_port);

		/* 创建新线程处理与该客户端的通信
		 * 线程函数: client_communication
		 * 线程参数: ci（客户端连接信息）
		 */
		if(pthread_create(&tid, NULL, client_communication, ci))
		{
			perror("pthread_create");
			close(sock_conn);
			free(ci);
			continue;
		}
	}

	/* 以下代码正常情况下不会执行（上方是死循环） */
	close(sock_listen);
}


/**
 * @brief 客户端通信线程函数
 *
 * 每个ESP8266设备连接都会创建一个独立线程运行此函数。
 * 主要职责：
 *   1. 按行读取设备上报的数据
 *   2. 解析自定义协议格式 "主题 载荷"
 *   3. 首次上线时将设备ID和socket注册到哈希表
 *   4. 将解析后的数据通过MQTT发布到对应主题
 *   5. 设备断开后从哈希表移除
 *
 * @param arg 指向client_info结构体的指针（包含socket、IP、端口）
 * @return NULL
 *
 * @note 线程以detach模式运行，结束后自动回收资源
 */
void* client_communication(void* arg)
{
	client_info* ci = arg;
	char dev_id[31];      /* 设备ID，最长30个字符 + '\0' */
	int is_first_online = 1;  /* 标记是否为首次上线，用于注册哈希表 */


	/* 将线程设置为detach模式
	 * 线程结束后自动释放资源，不需要其他线程调用pthread_join()
	 * 因为设备连接数量不确定，使用detach避免资源泄漏
	 */
	pthread_detach(pthread_self());


	/* 第5步：收发数据主循环 */
	char msg[1000];
	int ret;

	while(1)
	{
		/* 按行读取设备发送的数据
		 * recv_line()会逐字节读取，直到遇到'\n'为止
		 * 返回值: >0 读取的字节数, 0 对端关闭, -1 出错/超时
		 */
		ret = recv_line(ci->sock_conn, msg, sizeof(msg));

		if(ret <= 0) break;  /* 连接断开或出错，退出循环 */

		printf("\n终端设备说：%s\n", msg);

		/* 解析自定义协议格式："主题 载荷"
		 * 例如："wengyuanhang/sensor/Temp 28.22_65"
		 *       topic  = "wengyuanhang/sensor/Temp"
		 *       payload = "28.22_65"
		 */
		char* topic = NULL;
		char* payload = NULL;

		topic = strtok(msg, " ");    /* 第一次调用，以空格分隔，获取主题 */
		payload = strtok(NULL, " "); /* 第二次调用，获取载荷 */

		if(topic != NULL && payload != NULL)
		{
			/* 从主题中提取设备ID
			 * 主题格式: "设备ID/类别/属性"
			 * 例如: "wengyuanhang/sensor/Temp" → 设备ID = "wengyuanhang"
			 * strchr()找到第一个'/'的位置，其前面的部分就是设备ID
			 */
			char* p = strchr(topic, '/');
			int len;

			if(p == NULL) break;  /* 主题格式不合法，退出 */

			len = p - topic;  /* 设备ID的长度 */

			if(len > 30) break;  /* 设备ID过长，退出 */

			if(is_first_online)
			{
				/* 首次上线：提取设备ID并注册到哈希表
				 * 哈希表映射: 设备ID → socket描述符
				 * 后续收到MQTT控制命令时，通过设备ID查找socket转发
				 */
				strncpy(dev_id, topic, len);
				dev_id[len] = '\0';

				hash_table_insert(ci_table, dev_id, (void*)(long)(ci->sock_conn));
				is_first_online = 0;

				printf("设备上线: ID=%s, socket=%d\n", dev_id, ci->sock_conn);
			}
			else
			{
				/* 非首次上线：验证设备ID是否发生变化
				 * 同一个TCP连接不应该切换设备ID，如果发生变化说明协议异常
				 */
				if(strncmp(dev_id, topic, len) != 0) break;
			}

			/* 将传感器数据发布到MQTT服务器
			 * topic: MQTT主题（如 "wengyuanhang/sensor/Temp"）
			 * payload: MQTT载荷（如 "28.22_65"）
			 * MQTT Broker会将消息转发给所有订阅该主题的客户端（H5、小智等）
			 */
			mqttclient_pub_msg(&g_client_itmojun, topic, payload);
		}

	}


	/* 第6步：设备断开连接后的清理工作 */
	close(ci->sock_conn);
	printf("\n客户端(%s:%hu)断开连接！\n", ci->ip, ci->port);	

	free(ci);

	/* 设备离线后，从哈希表中移除该设备的映射关系
	 * 这样后续收到该设备的MQTT控制命令时，不会尝试向已断开的socket发送数据
	 */
	hash_table_remove(ci_table, dev_id);

	return NULL;
}


/**
 * @brief 按行读取TCP数据（以'\n'为分隔符）
 *
 * 逐字节从socket读取数据，直到遇到换行符'\n'为止。
 * 换行符不会被存入缓冲区，缓冲区以'\0'结尾。
 *
 * @param sock   套接字描述符
 * @param buf    接收缓冲区
 * @param maxlen 缓冲区最大长度
 * @return >0 实际读取的字节数（不含'\n'和'\0'）
 * @return 0  对端正常关闭连接（EOF，未读到任何数据）
 * @return -1 读取错误或超时
 *
 * @note 此函数是阻塞的，受setsockopt设置的SO_RCVTIMEO影响
 */
int recv_line(int sock, char* buf, int maxlen)
{
	int n, rc;
	char c;

	for(n = 1; n < maxlen; n++)
	{
		/* 每次读取1个字节 */
		if((rc = recv(sock, &c, 1, 0)) == 1)
		{
			if(c == '\n')
				break;  /* 遇到换行符，结束读取 */

			*buf++ = c;  /* 将非换行符字符存入缓冲区 */
		}
		else if(rc == 0)
		{
			if(n == 1)
				return 0;  /* 对端关闭连接，且未读到任何数据 */
			else
				break;     /* 对端关闭连接，但已读到部分数据 */
		}
		else
		{
			return -1;  /* 读取错误（包括超时 EAGAIN） */
		}
	}

	*buf = '\0';  /* 字符串结尾 */
	return n;     /* 返回读取的字节数 */
}
