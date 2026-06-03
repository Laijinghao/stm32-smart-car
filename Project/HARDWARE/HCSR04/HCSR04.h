#ifndef __HCSR04_H
#define __HCSR04_H
#include "sys.h"

// HC-SR04超声波：Trig=PA4 Echo=PA5, 范围2~400cm

void     HCSR04_Init(void);
uint16_t HCSR04_ReadDistance(void);

#endif
