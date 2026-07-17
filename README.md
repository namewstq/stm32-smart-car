# STM32 智能联网小车 / STM32 Smart Connected Car

![STM32](https://img.shields.io/badge/MCU-STM32F103VET6-blue)
![IDE](https://img.shields.io/badge/IDE-Keil%20MDK--ARM-green)
![Protocol](https://img.shields.io/badge/Protocol-MQTT-orange)
![Cloud](https://img.shields.io/badge/Cloud-Huawei%20IoT-red)

基于 **STM32F103VET6** 的多功能智能联网小车，集成多传感器融合、多模式控制、蓝牙/WiFi 双通信和华为云 IoT 平台接入。

---

## 📋 项目概述 / Overview

本项目设计并实现了一款具备 **环境感知、运动控制、无线通信和云端数据交互** 能力的智能联网小车。

**核心特性：**
- 🧠 **主控**：STM32F103VET6（Cortex-M3 @72MHz）
- 🚗 **运动**：L298N + 4路直流减速电机，PWM差速转向
- 👁️ **感知**：超声波HC-SR04、DHT11温湿度、4路红外传感器
- 📟 **显示**：0.96寸OLED（SSD1306，128×64，I2C）
- 📡 **通信**：HC-05蓝牙（USART2）+ ESP8266 WiFi（USART3）
- ☁️ **云端**：MQTT协议对接华为云IoT平台
- 🎵 **音频**：蜂鸣器播放13首预置旋律

---

## 🔧 硬件架构 / Hardware Architecture

### 引脚分配 / Pin Mapping

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
| 避障红外L/R | PC1/PC2 | GPIO上拉输入 |
| 巡线红外BL/BR | PG3/PG2 | GPIO上拉输入 |
| KEY1/KEY0 | PE3/PE4 | 外部中断 |
| 蜂鸣器 | PF0 | TIM1中断 |
| LED | PB5/PE5 | GPIO推挽输出 |

### 定时器分配 / Timer Allocation

| 定时器 | 用途 | 频率 | 中断 |
|--------|------|------|------|
| SysTick | 延时 | 1kHz | 无 |
| TIM1 | 蜂鸣器音乐 | 250μs中断 | 有 |
| TIM2 | 舵机PWM | 50Hz | 无 |
| TIM3 | 超声波计时 | 1μs/tick | 无 |
| TIM4 | 电机PWM | 1kHz | 无 |

---

## 📁 项目结构 / Project Structure

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
│   └── sys.c/h                 # NVIC配置
├── CMSIS/                      # ARM CMSIS 内核文件
├── STM32F10x_StdPeriph_Driver/ # STM32标准外设库
├── PROJ/                       # Keil MDK项目文件
│   └── project.uvprojx         # 项目工程文件
├── LIST/                       # 编译列表输出（gitignore）
├── OUT/                        # 编译目标输出（gitignore）
├── .gitignore
└── README.md
```

---

## 💻 软件架构 / Software Architecture

### 分层设计 / Layered Design

```
┌─────────────┐  应用层：主循环调度、多模式控制
├─────────────┤  通信层：MQTT、蓝牙协议
├─────────────┤  驱动层：硬件外设驱动
└─────────────┘  硬件层：STM32F103VET6
```

### 主循环调度 / Main Loop Schedule

每循环约 **40ms**，依次执行：

| 步骤 | 任务 | 频率 |
|------|------|------|
| 1 | DHT11温湿度采集 | 每600ms |
| 2 | 蓝牙指令解析 | 每次循环 |
| 3 | 按键检测 | 每次循环 |
| 4 | OLED刷新（脏标志） | 数据变化时 |
| 5 | 模式控制执行 | 每次循环 |
| 6 | 超声波测距 | 每次循环 |
| 7 | IoT通信（上报/心跳/命令） | 分频执行 |

### 5种工作模式 / 5 Operating Modes

| 模式 | 值 | 说明 |
|------|----|------|
| **遥控模式** | 0 | 蓝牙/云端远程控制，红外防撞保护 |
| **避障模式** | 1 | 超声波+红外检测，自动绕开障碍物 |
| **混合模式** | 2 | 遥控优先，超时自动切回避障 |
| **跟随模式** | 3 | 超声波+舵机扫描，保持安全距离 |
| **巡线模式** | 4 | 底部红外检测黑线，差速循迹 |

### 非阻塞通信 / Non-blocking Communication

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

WiFi初始化期间，蓝牙依然响应，避免"卡死"。

---

## ☁️ 华为云对接 / Huawei Cloud Integration

### 平台配置

| 参数 | 值 |
|------|-----|
| Broker | `efff2cb551.st1.iotda-device.cn-south-1.myhuaweicloud.com` |
| 端口 | 1883 |
| 设备ID | `6a4baaccc9429d337f57cf79_myNodeId` |
| 心跳周期 | 60秒 |
| 上报间隔 | ~8秒 |

### 数据上报格式

```json
{
  "services": [{
    "service_id": "smokeDetector",
    "properties": {
      "temperature": 28.0,
      "humidity": 65,
      "distance": 44.8,
      "mode": 0,
      "speed": 650
    }
  }]
}
```

### 支持的命令

运动控制：`goA/goB/goL/goR/stop`
模式切换：`mode0` ~ `mode4`
参数设置：`servo{angle}` `setspeed{speed}` `screen{page}`
其他：`beep` `music` `ledon/ledoff` `log` `getdata` `help`

---

## 🚀 快速开始 / Getting Started

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

1. **上电**：自动初始化，OLED显示开机动画
2. **蓝牙控制**：手机连接HC-05，发送指令
3. **模式切换**：发送 `mode0`~`mode4`
4. **WiFi连接**：自动连接WiFi（TQ 2805）
5. **云端控制**：华为云IoT平台下发命令
6. **OLED切换**：按KEY1或发 `screen0/1/2`

---

## 🧪 测试结果 / Test Results

| 测试项 | 结果 |
|--------|------|
| 电机驱动 | ✅ 正反转/差速转向正常 |
| 超声波测距 | ✅ 精度±0.3cm |
| DHT11温湿度 | ✅ 校验通过 |
| 红外避障 | ✅ 3次采样防抖 |
| 巡线功能 | ✅ 差速修正稳定 |
| OLED显示 | ✅ 3页面切换正常 |
| 蓝牙控制 | ✅ 10米内<100ms响应 |
| 华为云对接 | ✅ 上报成功率>99% |

---

## 📝 蓝牙指令集 / Bluetooth Commands

| 指令 | 功能 |
|------|------|
| goA / goB / goL / goR / stop | 运动控制 |
| mode0 ~ mode4 | 模式切换 |
| speedup / speeddown | 加减速 |
| setspeedXXX | 直接设速(0-999) |
| screen0 / 1 / 2 | OLED页面切换 |
| beepon / music / music off | 蜂鸣器/音乐 |
| log | 调试日志开关 |
| help | 帮助列表 |

---

## 👥 团队 / Team

| 角色 | 姓名 | 职责 |
|------|------|------|
| 组长 🧑‍💼 | **唐秋** | 系统集成、多模式控制、OLED显示、蜂鸣器音乐 |
| 组员 🔧 | **王宇** | 硬件驱动（电机、传感器、OLED、舵机、LED） |
| 组员 📡 | **税华桂** | 通信与云平台（ESP8266、MQTT、华为云、蓝牙） |
| 指导老师 👨‍🏫 | **李志凯** | 项目指导 |

---

## 📜 许可证 / License

本项目仅供学习交流使用。
