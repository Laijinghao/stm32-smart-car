# STM32 智能小车 — 红外循迹 + 超声波避障

> 基于 STM32F103C8T6 的四路红外循迹 + 超声波避障智能小车，支持蓝牙遥控、OLED 显示、舵机云台。

## 硬件清单

| 模块 | 型号 | 数量 |
|------|------|------|
| 主控 | STM32F103C8T6 (Cortex-M3) | 1 |
| 电机驱动 | TB6612FNG | 1 |
| 直流电机 | TT 马达 1:48 | 4 |
| 红外传感器 | TCRT5000 × 4 路 | 1 |
| 超声波 | HC-SR04 | 1 |
| 舵机 | SG90 (云台) | 1 |
| OLED | 0.96寸 I2C (SSD1306) | 1 |
| 蓝牙 | JDY-31 (BLE) | 1 |

## 引脚接线

| 功能 | 引脚 | 说明 |
|------|------|------|
| 电机方向 | PB5 PB6 PB7 PB8 | TB6612 IN1~IN4 |
| 电机 PWM | PA6 PA7 | TIM3_CH1 CH2, 5KHz |
| 红外 X1~X4 | PB0 PB1 PB10 PB11 | 0=踩线 1=白底 |
| 超声波 Trig | PA4 | 触发脚 |
| 超声波 Echo | PA5 | 回波脚 TIM4 计时 |
| 舵机 | PA0 | TIM2_CH1, 50Hz |
| OLED SCL | PA11 | 软件 I2C |
| OLED SDA | PA12 | 软件 I2C |
| 蓝牙 TX | PA9 | USART1, 9600bps |
| 蓝牙 RX | PA10 | USART1, 9600bps |
| 板载 LED | PC13 | 低电平亮 |

## 模式与蓝牙指令

| 指令 | 模式 | 说明 |
|------|------|------|
| `1` | 自动模式 | 无障碍 → 循迹，有障碍 → 避障（默认） |
| `2` | 纯循迹 | 四路红外跟踪黑线 |
| `3` | 纯避障 | 超声波测距 + 舵机扫描绕障 |
| `4` | 蓝牙遥控 | F=前 B=后 L=左 R=右 S=停 |

`1~4` 在任何模式下都生效。`F/B/L/R/S` 仅在蓝牙模式(4)下生效。

## 循迹算法

4 路红外传感器分级响应，9 种情况按优先级匹配：

```
[1 0 0 1] 完美居中 → 直走
[1 0 1 1] X2踩线  → 左微调 (差速)
[1 1 0 1] X3踩线  → 右微调 (差速)
[0 0 1 1] 左偏    → 急左转 (坦克式)
[1 1 0 0] 右偏    → 急右转 (坦克式)
[0 1 1 1] 极左偏  → 半速左转修正
[1 1 1 0] 极右偏  → 半速右转修正
3路以上踩线    → 十字路口，直走
[1 1 1 1] 离线    → 原地找线（超时2秒直走）
```

## 避障算法

1. 超声波测距 → 前方 < 25cm 触发
2. 舵机左扫(0°) → 右扫(180°) → 比较两侧距离
3. 选择空旷方向 (> 35cm) → 旋转 → 前进绕行
4. 两侧均堵 → 180° 掉头

## 编译与烧录

- IDE: Keil MDK v5
- 编译器: ARMCC V5.06
- 器件包: Keil.STM32F1xx_DFP.2.2.0
- 调试器: ST-Link / J-Link
- Flash 算法: STM32F10x Med-density Flash (128KB → 64KB)

### 常见问题

**Flash Download Failed - No Algorithm found**: Keil 工程中 Debug → Settings → Flash Download → Add → 选择 `STM32F10x_128.FLM`（Pack 目录下）

## 项目结构

```
Project/
├── USER/          主程序与控制逻辑
│   ├── main.c         主循环、模式切换、OLED显示
│   ├── control.c/h    循迹/避障/蓝牙控制算法
│   └── stm32f10x_it.c 中断服务函数
├── HARDWARE/      外设驱动
│   ├── LED/           LED 指示灯
│   ├── MOTOR/         电机 TB6612 驱动
│   ├── TIME/          TIM3 PWM 配置
│   ├── HCSR04/        超声波 HC-SR04
│   ├── Servo/         舵机 SG90
│   ├── Trace/         红外循迹传感器
│   ├── OLED/          OLED 显示 + 字库
│   └── USART/         蓝牙串口
├── SYSTEM/        系统延时
├── Library/       标准外设库 (STD Peripheral Library)
├── Start/         启动文件 + CMSIS
└── Objects/       编译输出
```

## 开发工具链

- 编辑器: VS Code / Keil MDK
- 编译器: ARM Compiler 5 (V5.06)
- 调试: ST-Link V2
- 串口调试: 任意蓝牙串口 APP (9600bps)
