#ifndef __LED_H
#define __LED_H

// ============================================================
// 板载LED指示灯驱动 (PC13)
// LED低电平点亮，高电平熄灭
// ============================================================

void LED_Init(void);       // 初始化LED对应的GPIO
void LED_ON(void);         // 点亮LED（PC13输出低电平）
void LED_OFF(void);        // 熄灭LED（PC13输出高电平）
void LED_Toggle(void);     // 翻转LED状态（亮→灭 或 灭→亮）
void LED_Flash(void);     // LED快闪两次，提示模式切换

#endif
