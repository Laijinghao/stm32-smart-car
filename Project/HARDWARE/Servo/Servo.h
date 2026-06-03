#ifndef __SERVO_H
#define __SERVO_H
#include "sys.h"

// ============================================================
// SG90舵机驱动（PWM控制角度）
//
// 接线：PA0 → TIM2_CH1 → 舵机信号线(橙色/白色)
//
// SG90舵机控制原理：
//   PWM频率50Hz（周期20ms）
//   0.5ms高电平 = 0度     1.0ms = 45度     1.5ms = 90度
//   2.0ms = 135度          2.5ms = 180度
// ============================================================

void Servo_Init(u16 arr, u16 psc);  // 初始化舵机PWM
void Servo_SetAngle(uint8_t angle); // 设置舵机角度：0~180度

#endif
