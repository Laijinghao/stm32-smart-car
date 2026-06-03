#ifndef __TIM3_PWM_H
#define __TIM3_PWM_H
#include "sys.h"

// ============================================================
// TIM3 PWM输出驱动（控制电机转速）
//   通道1 -> PA6  (右轮PWM)
//   通道2 -> PA7  (左轮PWM)
//   频率约5KHz (72MHz / (psc+1) / (arr+1))
// ============================================================

void TIM3_PWM_Init(u16 arr, u16 psc);

#endif
