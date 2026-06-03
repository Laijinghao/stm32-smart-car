#ifndef __TRACE_H
#define __TRACE_H
#include "stm32f10x.h"

// ============================================================
// 四路红外循迹传感器模块
//
// 传感器排列（从车头看，从左到右）：
//    X1(最左)  X2(中左)  X3(中右)  X4(最右)
//
// 接线：
//   X1 → PB0     X2 → PB1
//   X3 → PB10    X4 → PB11
//
// 读取规则：
//   传感器对着黑线 → 输出0（低电平）
//   传感器对着白底 → 输出1（高电平）
//
// 使用宏定义直接读取GPIO输入，方便循迹逻辑中快速判断
// ============================================================

// --- 4个传感器读值宏（返回0=检测到黑线，1=白底） ---
#define TRACE_X1  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0)   // 最左
#define TRACE_X2  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1)   // 中左
#define TRACE_X3  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10)  // 中右
#define TRACE_X4  GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11)  // 最右

void Trace_Init(void);  // 初始化4个循迹引脚（配置为内部上拉输入）

#endif
