/**
 * ============================================================
 * TIM3 双路PWM输出 —— 电机调速
 *
 * 【什么是PWM？】
 *   PWM = Pulse Width Modulation = 脉冲宽度调制
 *   简单说：给电机飞速地通电/断电（每秒几千次），
 *   通电时间占比越大 → 等效电压越高 → 电机转得越快。
 *
 *   【打个比方】
 *     你在炒菜，火太大想调小。但你只有一个开关（要么开要么关）。
 *     你怎么办？快速地开关开关开关——
 *       开1秒关1秒 = 50%火力
 *       开3秒关1秒 = 75%火力
 *       一直开着    = 100%火力
 *     PWM就是这个道理，但频率极快（5KHz=每秒5000次），
 *     电机感觉不到通断，只感觉到"平均电压"。
 *
 * 【为什么频率选5KHz？】
 *   - 太低(<1KHz)：人耳能听到电机发出的高频啸叫，很刺耳
 *   - 太高(>20KHz)：电机来不及响应，而且开关损耗大
 *   - 5KHz刚好在人耳听觉范围外，电机也能很好响应
 *
 * 【TIM3通道分配】
 *   PA6 → TIM3_CH1 → 右轮PWM (接TB6612的PWMB)
 *   PA7 → TIM3_CH2 → 左轮PWM (接TB6612的PWMA)
 *
 * 【频率计算（以 arr=7199, psc=1 为例）】
 *   定时器计数频率 = 72MHz / (psc+1) = 72MHz / 2 = 36MHz
 *   PWM频率 = 36MHz / (arr+1) = 36M / 7200 = 5KHz ✓
 *
 *   【注意】72MHz / (PSC+1) 不能超过定时器的最大频率，
 *   STM32F103的TIM最大是72MHz，所以PSC最小可以填0。
 *
 * 【占空比控制】
 *   CCR(比较值)决定了占空比：
 *     计数值 0 → ... → CCR → ... → ARR → 0
 *                └─高电平─┘   └─低电平─┘
 *   占空比 = CCR / (ARR+1) × 100%
 *   CCR=3600: 3600/7200 = 50% 半速
 *   CCR=7200: 7200/7200 = 100% 全速
 *   CCR=0:    0/7200 = 0% 停转
 * ============================================================
 */

#include "TIM3_PWM.h"


/**
 * TIM3_PWM_Init —— 初始化TIM3双路PWM
 *
 * @param arr : ARR值，决定PWM频率
 * @param psc : PSC值，对72MHz时钟预分频
 *
 * 调用示例：TIM3_PWM_Init(7199, 1) → 5KHz PWM
 */
void TIM3_PWM_Init(u16 arr, u16 psc)
{
	// --- 开时钟 ---
	// TIM3在APB1上，GPIOA在APB2上
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,  ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// --- PA6和PA7都配成"复用推挽输出" ---
	// 复用 = 引脚控制权给定时器（自动输出PWM波形，不需要软件干预）
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;   // 同时配置两个脚
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;            // 复用推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// --- 时基（决定PWM频率） ---
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period        = arr;              // ARR
	TIM_TimeBaseStructure.TIM_Prescaler     = psc;              // PSC
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	// --- PWM输出配置 ---
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
	// PWM1模式：计数值 < CCR → 高电平，适合电机控制
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;

	// 把同样的配置写到两个通道
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);   // CH1 → PA6 → 右轮
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);   // CH2 → PA7 → 左轮

	// 启用预装载（防止中途改变CCR导致PWM波形错乱）
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

	// --- 启动TIM3 ---
	TIM_Cmd(TIM3, ENABLE);
}
