#ifndef __DELAY_H
#define __DELAY_H
#include "sys.h"

// ============================================================
// 延时函数声明
// 基于SysTick系统定时器实现微秒/毫秒/秒级延时
// ============================================================

void Delay_us(uint32_t us);   // 微秒级延时，最大约233015us
void Delay_ms(uint32_t ms);   // 毫秒级延时
void Delay_s(uint32_t s);     // 秒级延时

#endif
