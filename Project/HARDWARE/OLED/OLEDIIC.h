#ifndef __OLEDIIC_H
#define __OLEDIIC_H
#include "sys.h"

// ============================================================
// 软件模拟I2C通信（用于0.96寸OLED屏幕）
//
// 为什么用软件模拟？
//   因为STM32的硬件I2C比较复杂且容易出bug，用GPIO模拟更可靠。
//
// 接线：
//   SCL → PA11  (时钟线)
//   SDA → PA12  (数据线)
//
// I2C是一个"两线制"的串行通信协议：
//   - SCL：时钟线，由主机产生时钟节拍
//   - SDA：数据线，数据在这根线上传输
// ============================================================

// --- 宏定义：操作SCL和SDA引脚 ---
#define OLED_SCL_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_11)   // SCL拉低
#define OLED_SCL_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_11)     // SCL拉高
#define OLED_SDA_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_12)   // SDA拉低
#define OLED_SDA_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_12)     // SDA拉高

// --- I2C基本操作函数 ---
void I2C_Start(void);        // 发送起始信号
void I2C_Stop(void);         // 发送停止信号
void I2C_WaitAck(void);      // 等待从机应答
void I2C_SendByte(u8 dat);   // 发送一个字节数据

#endif
