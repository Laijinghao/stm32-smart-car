#ifndef __USART1_H
#define __USART1_H
#include "sys.h"

// ============================================================
// 串口1驱动（用于JDY-31蓝牙模块通信）
//
// 接线：
//   PA9  = TX  (STM32发送 → 蓝牙模块接收)
//   PA10 = RX  (STM32接收 ← 蓝牙模块发送)
//
// 波特率：9600bps（与手机蓝牙APP一致）
//
// JDY-31蓝牙模块：
//   通电后自动进入AT模式或透传模式
//   手机通过蓝牙串口APP发送字符，STM32串口接收
//   收到的数据存在全局变量 RxData 中
// ============================================================

extern volatile u8 RxData;  // 串口最新收到的1字节数据

void USART1_Init(uint32_t baudRate);   // 初始化串口1
void USART1_SendByte(u8 byte);         // 发送1字节
void USART1_SendString(char *str);     // 发送字符串

#endif
