# Smart Classroom Device

智慧教室物联网终端 — 基于 STM32F103CBT6 的环境监测、设备控制与AI语音交互系统

---

## 📋 项目简介

本项目是一个智慧教室物联网终端设备，基于 STM32F103CBT6 微控制器，实现环境监测（温湿度、光照）、LED 设备控制、OLED 实时显示、云端数据交互，并支持通过小智 AI 语音助手进行远程查询与控制。

---

## 🎯 功能特性

| 功能 | 状态 | 说明 |
|------|------|------|
| **温湿度监测** | ✅ 已实现 | DHT11 数字温湿度传感器 |
| **光照监测** | ✅ 已实现 | 光敏电阻 + ADC 采集 |
| **LED 控制** | ✅ 已实现 | 2 路 LED，支持云端/语音控制 |
| **OLED 显示** | ✅ 已实现 | 0.96 寸 I2C 屏幕，实时显示所有数据 |
| **串口通信** | ✅ 已实现 | UART2 与 ESP8266 通信，115200bps |
| **云端数据上报** | ✅ 已实现 | STM32 → ESP8266 → TCP 服务器 → MQTT |
| **远程控制** | ✅ 已实现 | H5 页面 / 小智 AI → MQTT → TCP 服务器 → ESP8266 → STM32 |
| **AI 语音控制** | ✅ 已实现 | 通过小智 MCP 服务器控制 LED、查询传感器数据 |

---

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              完整通信链路                                    │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  H5/Web页面  │  │  小智AI助手  │  │  手机App    │
└──────┬──────┘  └──────┬──────┘  └──────┬──────┘
       │                │                │
       │ MQTT           │ MCP+MQTT       │ MQTT
       │                │                │
┌──────▼────────────────▼────────────────▼────────┐
│              MQTT 服务器 (Mosquitto)              │
│              阿里云 39.104.71.92:1883             │
└──────────────────────┬──────────────────────────┘
                       │
                       │ MQTT (订阅 +/cmd，发布传感器数据)
                       │
┌──────────────────────▼──────────────────────────┐
│          设备云主机 (阿里云 TCP 服务器)            │
│          device_cloud-master (端口 8866)          │
│          TCP Server + MQTT Client 双重身份        │
└──────────────────────┬──────────────────────────┘
                       │
                       │ TCP:8866
                       │
               ┌───────▼───────┐
               │   ESP8266     │
               │  (WiFi模块)   │
               └───────┬───────┘
                       │
                       │ UART:115200
                       │
┌──────────────────────▼──────────────────────────┐
│              STM32F103CBT6                       │
│                                                  │
│   ┌──────┐  ┌──────┐  ┌──────────┐  ┌───────┐  │
│   │ LED1 │  │ LED2 │  │  DHT11   │  │ 光照  │  │
│   │(高亮)│  │(低亮)│  │温湿度传感器│  │ 传感器│  │
│   └──────┘  └──────┘  └──────────┘  └───────┘  │
│                                                  │
│   ┌──────────────┐                               │
│   │   OLED 显示   │                               │
│   └──────────────┘                               │
└──────────────────────────────────────────────────┘
```

---

## 📡 通信协议

### MQTT 主题定义

| 主题 | 方向 | 数据格式 | 说明 |
|------|------|---------|------|
| `wengyuanhang/sensor/Temp` | STM32→云端 | `温度_湿度` | 温湿度数据，如 `28.22_65` |
| `wengyuanhang/sensor/Light` | STM32→云端 | `光照值` | 光照强度，如 `1234` |
| `wengyuanhang/state/Led1` | STM32→云端 | `0` 或 `1` | LED1 状态（1=亮，0=灭） |
| `wengyuanhang/state/Led2` | STM32→云端 | `0` 或 `1` | LED2 状态（0=亮，1=灭） |
| `wengyuanhang/cmd` | 云端→STM32 | 单字符命令 | 控制指令 |

### 下行控制命令 (云端 → STM32)

| 命令字符 | 功能 | LED1 (高电平亮) | LED2 (低电平亮) |
|----------|------|-----------------|-----------------|
| `'a'` | 开灯 | SET → 亮 | RESET → 亮 |
| `'b'` | 关灯 | RESET → 灭 | SET → 灭 |

> **LED 极性说明**：LED1 高电平有效（GPIO=1 亮），LED2 低电平有效（GPIO=0 亮）

### 上行数据示例

```
wengyuanhang/sensor/Temp 28.22_65
wengyuanhang/sensor/Light 1234
wengyuanhang/state/Led1 1
wengyuanhang/state/Led2 0
```

---

## 🔌 硬件连接

### STM32F103CBT6 引脚定义

| 功能 | 引脚 | 说明 |
|------|------|------|
| **LED1** | PB0 | LED1 控制，高电平亮 |
| **LED2** | PB1 | LED2 控制，低电平亮 |
| **UART2_TX** | PA2 | 发送到 ESP8266 |
| **UART2_RX** | PA3 | 接收来自 ESP8266 |
| **I2C_SCL** | PB6 | OLED 时钟线 |
| **I2C_SDA** | PB7 | OLED 数据线 |
| **ADC_IN** | PA0 | 光照传感器输入 |
| **DHT11** | PA1 | 温湿度传感器数据线 |

### 通信参数

| 参数 | 值 |
|------|-----|
| UART 波特率 | 115200 |
| UART 数据位 | 8 |
| UART 停止位 | 1 |
| UART 校验位 | 无 |
| TCP 端口 | 8866 |
| MQTT 端口 | 1883 |

---

## 📁 项目结构

```
smart_cbt6/
├── Core/
│   ├── Inc/                  # 头文件
│   │   ├── main.h
│   │   ├── usart.h
│   │   ├── adc.h
│   │   ├── i2c.h
│   │   ├── gpio.h
│   │   ├── light.h           # 光照传感器
│   │   ├── dht11.h           # DHT11 温湿度传感器
│   │   ├── ds18b20.h         # DS18B20 (预留，未启用)
│   │   └── oled.h            # OLED 显示
│   └── Src/                  # 源文件
│       ├── main.c            # 主程序：传感器采集、数据上报、OLED 显示
│       ├── usart.c           # 串口通信：接收控制命令中断回调
│       ├── adc.c
│       ├── i2c.c
│       ├── gpio.c
│       ├── light.c
│       ├── dht11.c
│       ├── ds18b20.c
│       ├── oled.c
│       └── stm32f1xx_it.c
├── Drivers/                  # HAL 驱动库
├── MDK-ARM/                  # Keil 工程文件
├── smart_cbt6.ioc            # STM32CubeMX 配置文件
│
├── device_cloud-master/      # 设备云主机 (部署在阿里云服务器)
│   ├── main.c                # 入口：初始化 MQTT + 启动 TCP 服务器
│   ├── device_server.c       # TCP 服务器：监听 8866，接收 ESP8266 数据
│   ├── device_server.h
│   ├── mqtt_client.c         # MQTT 客户端：订阅 +/cmd，转发控制指令
│   ├── mqtt_client.h
│   ├── hash_table.c          # 哈希表：设备 ID → socket 连接映射
│   ├── hash_table.h
│   └── Makefile              # 编译：gcc -o ds -pthread -lpaho-mqtt3a
│
└── mcp-xiaozhi-new/          # 小智 AI MCP 服务器 (部署在阿里云服务器)
    ├── my_mcp_server.py      # MCP 服务器：MQTT 订阅传感器数据 + 提供控制工具
    ├── mcp_pipe.py           # WebSocket ↔ stdio 管道，连接小智平台
    ├── mcp_config.json       # MCP 服务器配置
    ├── requirements.txt      # Python 依赖
    └── calculator/           # 计算器示例工具
```

---

## 🚀 部署与启动

### 一、STM32 终端设备

1. 使用 STM32CubeMX 打开 `smart_cbt6.ioc` 配置引脚
2. 使用 Keil MDK-ARM 打开 `MDK-ARM/smart_cbt6.uvprojx`
3. 编译 (F7) 并烧录 (F8)
4. ESP8266 需预先配置好 WiFi 连接和 TCP 客户端模式，连接阿里云服务器 8866 端口

### 二、设备云主机 (阿里云服务器)

```bash
# 1. 安装依赖
apt-get update
apt-get install -y libpaho-mqtt3a-dev gcc make mosquitto

# 2. 启动 MQTT 服务
systemctl start mosquitto
systemctl enable mosquitto

# 3. 编译设备服务器
cd ~/device_cloud
make

# 4. 前台测试
./ds
# 看到 "Successful connection" 和 "Subscribe succeeded" 即成功

# 5. 后台运行
nohup ./ds > ds.log 2>&1 &

# 6. 验证
ps aux | grep ds
netstat -tlnp | grep 8866
```

**阿里云安全组**需开放以下端口：

| 端口 | 协议 | 用途 |
|------|------|------|
| 8866 | TCP | ESP8266 连接设备服务器 |
| 1883 | TCP | MQTT 通信 |
| 80/443 | TCP | H5 页面访问（如需要） |

### 三、小智 AI MCP 服务器 (阿里云服务器)

```bash
# 1. 安装 Python 依赖
cd ~/mcp-xiaozhi-new
pip3 install -r requirements.txt

# 2. 设置小智平台 WebSocket 端点
export MCP_ENDPOINT="ws://你的小智平台MCP端点地址"

# 3. 启动（同时启动 device-controller 和 calculator）
python3 mcp_pipe.py

# 4. 或只启动设备控制器
python3 mcp_pipe.py device-controller

# 5. 后台运行
nohup python3 mcp_pipe.py > mcp.log 2>&1 &
```

### 四、启动顺序

```
1. Mosquitto MQTT 服务    ← systemctl start mosquitto
2. 设备云主机 (ds)         ← nohup ./ds > ds.log 2>&1 &
3. MCP 服务器 (mcp_pipe)   ← nohup python3 mcp_pipe.py > mcp.log 2>&1 &
4. STM32 + ESP8266 上电
```

---

## 🎤 小智 AI 语音控制

MCP 服务器为小智 AI 提供以下工具：

### 控制类工具

| 语音指令 | 调用工具 | MQTT 命令 | 效果 |
|---------|---------|-----------|------|
| "开灯"、"打开灯" | `led_on()` | `wengyuanhang/cmd` → `a` | LED1 亮，LED2 亮 |
| "关灯"、"关闭灯" | `led_off()` | `wengyuanhang/cmd` → `b` | LED1 灭，LED2 灭 |
| "打开蜂鸣器" | `buzzer_on()` | `wengyuanhang/cmd` → `c` | 蜂鸣器开 |
| "关闭蜂鸣器" | `buzzer_off()` | `wengyuanhang/cmd` → `d` | 蜂鸣器关 |
| "打开风扇" | `fan_on()` | `wengyuanhang/cmd` → `e` | 风扇开 |
| "关闭风扇" | `fan_off()` | `wengyuanhang/cmd` → `f` | 风扇关 |
| "打开智能插座" | `socket_on()` | `wengyuanhang/cmd` → `a1` | 插座通电 |
| "关闭智能插座" | `socket_off()` | `wengyuanhang/cmd` → `b1` | 插座断电 |

### 查询类工具

| 语音指令 | 调用工具 | 返回示例 |
|---------|---------|---------|
| "温度多少"、"湿度多少" | `query_temperature_humidity()` | "当前温度 28.22°C，湿度 65%" |
| "光照多少"、"亮度" | `query_light()` | "当前光照强度 1234，正常" |
| "灯的状态"、"灯亮不亮" | `query_led_state()` | "LED1 当前亮，LED2 当前亮" |
| "插座功率" | `query_socket_power()` | "当前电压 220V，电流 0.5A，功率 110W" |
| "设备状态"、"全部状态" | `query_all_status()` | "温度 28.22°C，湿度 65%；光照 1234；LED1 亮，LED2 亮" |

---

## 📺 OLED 显示内容

```
┌────────────────────────┐
│   Smart Device         │  第0行 - 标题
│   ----------------     │  第2行 - 分隔线
│   Temp: 28.22          │  第3行 - 温度
│   Humi: 65             │  第4行 - 湿度
│   Light: 1234          │  第5行 - 光照
│   LED1: ON             │  第6行 - LED1 状态
│   LED2: ON             │  第7行 - LED2 状态
└────────────────────────┘
```

---

## 🔄 数据流程

### 上行：传感器数据 (STM32 → 云端)

```
1. 主循环每 1 秒采集一次
2. DHT11 读取温湿度 → UART 发送 "wengyuanhang/sensor/Temp 28.22_65\n"
3. ADC 读取光照值   → UART 发送 "wengyuanhang/sensor/Light 1234\n"
4. GPIO 读取 LED1   → UART 发送 "wengyuanhang/state/Led1 1\n"
5. GPIO 读取 LED2   → UART 发送 "wengyuanhang/state/Led2 0\n"
6. ESP8266 通过 TCP 转发到阿里云 8866 端口
7. TCP 服务器解析 "主题 载荷" 格式，MQTT 发布到对应主题
8. H5/MCP 订阅对应主题获取数据
```

### 下行：控制命令 (云端 → STM32)

```
1. H5 页面或小智 AI 发布命令到 MQTT 主题 "wengyuanhang/cmd"
2. TCP 服务器订阅 "+/cmd" 收到命令（如 "a"）
3. 从主题提取设备 ID "wengyuanhang"，查哈希表找到 socket
4. TCP 服务器通过 8866 端口将 "a" 转发给 ESP8266
5. ESP8266 通过 UART 将 'a' 发送给 STM32
6. STM32 触发 UART 接收中断，回调函数执行 LED 控制
7. 下一轮主循环上报更新后的 LED 状态
```

---

## ⚙️ 配置说明

### 修改设备 ID

在 `Core/Src/main.c` 中：

```c
#define DEVICE_ID "wengyuanhang"
```

> 设备 ID 在整个系统中必须保持一致（STM32、TCP 服务器哈希表、MQTT 主题）

### 修改 MQTT 服务器地址

在 `device_cloud-master/mqtt_client.c` 中：

```c
#define ADDRESS_ITMOJUN  "tcp://127.0.0.1:1883"
```

### 修改 TCP 监听端口

在 `device_cloud-master/device_server.c` 中：

```c
myaddr.sin_port = htons(8866);
```

---

## 📝 扩展开发

### 扩展新的控制命令

1. 在 `Core/Src/usart.c` 的 `HAL_UART_RxCpltCallback` 中添加：

```c
else if(cmd == 'c'){
    // 添加新硬件控制
}
```

2. 在 `mcp-xiaozhi-new/my_mcp_server.py` 中添加对应的 MCP 工具：

```python
@mcp.tool()
def new_device_on() -> dict:
    """描述：当用户说'xxx'时调用此工具。"""
    success = publish_mqtt_message(MQTT_CMD_TOPIC, "c")
    return {"success": success, "message": "设备已打开" if success else "操作失败"}
```

### 扩展新的传感器

1. 在 `Core/Inc/` 添加传感器头文件
2. 在 `Core/Src/` 添加传感器实现
3. 在 `main.c` 中包含头文件、调用采集函数、格式化上报
4. 在 `my_mcp_server.py` 中添加对应的 MQTT 订阅和查询工具

---

## ⚠️ 注意事项

1. ESP8266 需预先配置好 WiFi 连接和 TCP 客户端模式
2. 设备云主机需同时运行 TCP 服务和 MQTT 客户端（编译后的 `ds` 程序已包含两者）
3. MQTT 服务器地址在 `mqtt_client.c` 中配置，默认连接本机 `127.0.0.1:1883`
4. 设备 ID 需在整个系统中保持唯一
5. LED2 为低电平有效，状态上报值 0 表示亮，1 表示灭
6. MCP 服务器和设备云主机可部署在同一台阿里云服务器上
7. MCP 客户端 ID 不能与设备云主机的 MQTT 客户端 ID 重复

---

## 👥 作者

wengyuanhang
