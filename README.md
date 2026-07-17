# STM32 智能联网小车

![STM32](https://img.shields.io/badge/MCU-STM32F103VET6-blue)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK--ARM-green)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-orange)
![Cloud](https://img.shields.io/badge/Cloud-%E5%8D%8E%E4%B8%BA%E4%BA%91IoT-red)

基于 **STM32F103VET6** 的多功能智能联网小车，集成多传感器融合、5种工作模式、蓝牙/WiFi 双通信和华为云 IoT 平台接入。

---

## 项目概述

本项目设计并实现了一款具备**环境感知、运动控制、无线通信和云端数据交互**能力的智能联网小车。

**核心特性：**
- **主控**：STM32F103VET6（Cortex-M3 @72MHz）
- **运动**：L298N + 4路直流减速电机，PWM差速转向
- **感知**：超声波HC-SR04、DHT11温湿度、4路红外传感器（2路避障+2路巡线）
- **显示**：0.96寸OLED（SSD1306，128×64，I2C）
- **通信**：HC-05蓝牙（USART2）+ ESP8266 WiFi（USART3）
- **云端**：MQTT协议对接华为云IoT平台
- **音频**：蜂鸣器TIM1中断驱动，13首预置旋律

---

## 硬件架构

### 引脚分配

| 模块 | 引脚 | 功能 |
|------|------|------|
| 左电机方向 | PB12/PB13 | GPIO推挽输出 |
| 右电机方向 | PB14/PB15 | GPIO推挽输出 |
| 左电机PWM | PB6 (TIM4_CH1) | 1kHz PWM |
| 右电机PWM | PB7 (TIM4_CH2) | 1kHz PWM |
| 舵机 | PA0 (TIM2_CH1) | 50Hz PWM |
| 超声波Trig | PA7 | GPIO输出 |
| 超声波Echo | PA6 | GPIO输入 |
| DHT11 | PA1 | 单总线 |
| OLED SCL | PG12 | I2C时钟 |
| OLED SDA | PD5 | I2C数据 |
| OLED RES | PD4 | GPIO输出 |
| 蓝牙TX/RX | PA2/PA3 | USART2 @9600 |
| ESP8266 TX/RX | PB10/PB11 | USART3 @115200 |
| 避障红外左/右 | PC1/PC2 | GPIO上拉输入 |
| 巡线红外左下/右下 | PG3/PG2 | GPIO上拉输入 |
| KEY1/KEY0 | PE3/PE4 | 外部中断 |
| 蜂鸣器 | PF0 | TIM1中断驱动 |
| LED | PB5/PE5 | GPIO推挽输出 |

### 定时器分配

| 定时器 | 用途 | 频率 | 中断 |
|--------|------|------|------|
| SysTick | 延时 | 1kHz | 无 |
| TIM1 | 蜂鸣器音乐 | 250μs中断 | 有 |
| TIM2 | 舵机PWM | 50Hz | 无 |
| TIM3 | 超声波计时 | 1μs/tick | 无 |
| TIM4 | 电机PWM | 1kHz | 无 |

---

## 项目结构

```
├── USER/                       # 用户应用层
│   ├── main.c                  # 主程序（初始化 + 主循环）
│   ├── stm32f10x_conf.h        # 外设配置头文件
│   └── stm32f10x_it.c/h        # 中断服务函数
├── HARDWARE/                   # 硬件驱动层
│   ├── motor.c/h               # 电机驱动（PWM + 方向）
│   ├── hcsr04.c/h              # HC-SR04超声波测距
│   ├── dht11.c/h               # DHT11温湿度
│   ├── servo.c/h               # SG90舵机
│   ├── oled.c/h                # SSD1306 OLED I2C驱动
│   │   ├── oledfont.h          # 字库
│   │   └── bmp.h               # 图片数据
│   ├── esp8266.c/h             # ESP8266 AT指令驱动
│   ├── esp8266_mqtt.c/h        # MQTT + 华为云对接
│   ├── usart.c/h               # 串口（USART1/2/3）
│   ├── key.c/h                 # 按键外部中断
│   ├── beep.c/h                # 蜂鸣器音乐引擎
│   ├── music.c/h               # 音乐播放
│   ├── led.c/h                 # LED指示灯
│   └── delay.c/h               # 延时函数
├── system/                     # 系统配置
│   └── sys.c/h                 # NVIC优先级配置
├── CMSIS/                      # ARM CMSIS 内核文件
├── STM32F10x_StdPeriph_Driver/ # STM32标准外设库
├── PROJ/                       # Keil MDK项目文件
│   └── project.uvprojx         # 项目工程文件
├── LICENSE                     # MIT许可证
└── README.md
```

---

## 软件架构

### 分层设计

```
应用层：主循环调度、5种模式控制
通信层：MQTT协议、蓝牙串口协议
驱动层：硬件外设驱动（电机、传感器、OLED等）
硬件层：STM32F103VET6 + 外设
```

### 主循环调度

每循环约 **40ms**，依次执行：

| 步骤 | 任务 | 频率 |
|------|------|------|
| 1 | DHT11温湿度采集 | 每600ms采集一次 |
| 2 | 蓝牙指令解析 | 每次循环 |
| 3 | 按键检测 | 每次循环 |
| 4 | OLED刷新（脏标志机制） | 仅数据变化时刷新 |
| 5 | 5种模式控制逻辑 | 每次循环 |
| 6 | 超声波测距 | 每次循环 |
| 7 | IoT通信（上报/心跳/命令） | 分频执行 |

### 5种工作模式

| 模式 | 值 | 说明 |
|------|----|------|
| **遥控模式** | 0 | 蓝牙/云端远程控制，红外防撞保护 |
| **避障模式** | 1 | 超声波+红外检测，自动绕开障碍物 |
| **混合模式** | 2 | 遥控优先，超时自动切回避障 |
| **跟随模式** | 3 | 超声波+舵机扫描，保持安全距离 |
| **巡线模式** | 4 | 底部红外检测黑线，差速循迹 |

### 非阻塞通信

WiFi初始化期间蓝牙依然响应，避免"卡死"：

```c
void delay_parse(uint32_t ms)
{
    while (ms >= 10) {
        delay_ms(10);
        parse_cmd();   // 每10ms检查蓝牙指令
        ms -= 10;
    }
}
```

---

## 华为云对接

### 平台配置

| 参数 | 说明 |
|------|------|
| Broker | `YOUR_BROKER_ADDRESS`（替换为平台接入地址） |
| 端口 | 1883 |
| 设备ID | `YOUR_DEVICE_ID`（替换为你的设备ID） |
| 心跳周期 | 60秒 |
| 上报间隔 | ~8秒 |
| WiFi | `YOUR_WIFI_SSID` / `YOUR_WIFI_PASSWORD`（替换） |

### 数据上报格式

```json
{
  "services": [{
    "service_id": "smokeDetector",
    "properties": {
      "temperature": 28.0,    /* 温度 */
      "humidity": 65,         /* 湿度 */
      "distance": 44.8,       /* 距离(cm) */
      "mode": 0,              /* 模式 0-4 */
      "speed": 650            /* 速度 0-999 */
    }
  }]
}
```

### 支持的命令

| 命令 | 功能 |
|------|------|
| `goA/goB/goL/goR/stop` | 运动控制（前进/后退/左转/右转/停止） |
| `mode0` ~ `mode4` | 切换5种工作模式 |
| `servo{angle}` | 舵机转到指定角度 |
| `setspeed{speed}` | 设置速度(0-999) |
| `screen{0/1/2}` | 切换OLED显示页面 |
| `beepon/music/music off` | 蜂鸣器/音乐控制 |
| `ledon/ledoff` | LED开关 |
| `log` | 调试日志开关 |
| `getdata` | 主动查询传感器数据 |
| `help` | 显示帮助列表 |

---

## 快速开始

### 环境要求

- **IDE**：Keil MDK-ARM V5+
- **编译器**：ARM Compiler V5
- **调试器**：ST-Link V2
- **MCU**：STM32F103VET6

### 编译步骤

1. 打开 `PROJ/project.uvprojx`
2. 选择目标芯片：STM32F103VE
3. 编译：Project → Rebuild all target files
4. 下载：Flash → Download

### 使用说明

1. **上电**：系统自动初始化，OLED显示开机动画
2. **蓝牙控制**：手机连接HC-05蓝牙模块，发送指令
3. **模式切换**：发送 `mode0`~`mode4`
4. **WiFi连接**：自动连接配置的WiFi，约10秒
5. **云端控制**：华为云IoT平台可下发命令、接收数据
6. **OLED切换**：按KEY1或发送 `screen0/1/2`
7. **紧急停止**：按KEY0或发送 `stop`

---

## 测试结果

| 测试项 | 结果 |
|--------|------|
| 电机驱动 | 正反转/差速转向正常 |
| 超声波测距 | 精度±0.3cm |
| DHT11温湿度 | 数据校验通过 |
| 红外避障 | 3次采样防抖，响应灵敏 |
| 巡线功能 | 差速修正稳定，直线循迹良好 |
| OLED显示 | 3页面切换正常，刷新流畅 |
| 蓝牙控制 | 10米内响应时间<100ms |
| 华为云对接 | 数据上报成功率>99% |

---

---

## 许可证

本项目仅供学习交流使用，基于 MIT 协议开源。
