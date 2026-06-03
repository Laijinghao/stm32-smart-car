/**
 * ============================================================
 * TB6612 双路电机驱动
 *
 * 【TB6612是什么？】
 *   东芝出的电机驱动芯片，一块芯片能同时控制两个直流电机。
 *   比老式的L298N好用：发热小、不用散热片、体积小。
 *
 * 【每个电机需要3根线控制】
 *   以左电机为例：
 *     AIN1(方向1)  AIN2(方向2)  PWMA(速度)
 *     ─────────────────────────────────────
 *       0            0           X       → 刹车（电机两端短路制动）
 *       0            1           PWM     → 正转（前进）
 *       1            0           PWM     → 反转（后退）
 *       1            1           X       → 停止（惯性滑行）
 *
 *   【刹车 vs 停止 有什么区别？】
 *     刹车(0,0)：芯片把电机两根线短接在一起，
 *               电机转动会产生反电动势，被短路线消耗掉，
 *               就像捏住车轮一样，很快停下来。
 *     停止(1,1)：芯片断开电机供电，
 *               电机靠惯性继续转，慢慢停下来。
 *
 * 【引脚连接（本项目）】
 *   左电机：PB5=AIN1  PB6=AIN2  PA7=PWMA(TIM3_CH2)
 *   右电机：PB7=BIN1  PB8=BIN2  PA6=PWMB(TIM3_CH1)
 *
 * 【PWM如何控制速度？】
 *   PWM = 脉冲宽度调制
 *   简单理解：给电机快速地通断电（比如每秒5000次）
 *   通电时间越长 → 等效电压越高 → 电机转得越快
 *   占空比 = 通电时间 / 周期 × 100%
 *   比如 TIM_SetCompare2(TIM3, 3600)：
 *     ARR=7199，所以 3600/7200 = 50%占空比，半速
 *
 *   这里的API设计中，Motor_SetSpeed的输入就是CCR值，范围 0~7200：
 *     7200 = 100%全速    3600 = 50%    1800 = 25%    0 = 停
 * ============================================================
 */

#include "stm32f10x.h"
#include "MOTOR.h"
#include "TIM3_PWM.h"


/**
 * Motor_GPIO_Init —— 初始化电机的4个方向控制引脚
 *
 * PB5~PB8 全配成推挽输出。
 * 推挽输出 = 能主动输出高电平和低电平，驱动能力较强。
 */
void Motor_GPIO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;    // 推挽输出
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/**
 * Motor_SetSpeed —— 同时控制两个电机的转速和方向
 *
 * @param leftSpeed  左轮：正=前进  负=后退  0=刹车
 * @param rightSpeed 右轮：正=前进  负=后退  0=刹车
 *
 * 【使用示例】
 *   Motor_SetSpeed(3500, 3500)   → 两轮同速前进 = 直走
 *   Motor_SetSpeed(2000, 3500)   → 左慢右快 = 车头向左转
 *   Motor_SetSpeed(-2800, 2800)  → 左轮后退、右轮前进 = 原地左转(坦克式)
 *   Motor_SetSpeed(0, 0)         → 两轮刹车 = 急停
 *
 * 【为什么左电机PA7是CH2而不是CH1？】
 *   因为硬件接线就是PA7→PWMA，PA6→PWMB。
 *   PA6是TIM3_CH1，PA7是TIM3_CH2。
 *   写代码时要对着原理图来，不能想当然。
 */
void Motor_SetSpeed(int leftSpeed, int rightSpeed)
{
	// ==================== 左电机 ====================
	// PA7 = TIM3_CH2(PWM)   PB5=AIN1   PB6=AIN2

	if (leftSpeed > 0)
	{
		// 正转：AIN2=1 AIN1=0
		TIM_SetCompare2(TIM3, leftSpeed);          // 输出PWM，占空比=leftSpeed/7200
		GPIO_SetBits(GPIOB, GPIO_Pin_6);           // AIN2 = 1
		GPIO_ResetBits(GPIOB, GPIO_Pin_5);         // AIN1 = 0
	}
	else if (leftSpeed < 0)
	{
		// 反转：AIN2=0 AIN1=1
		TIM_SetCompare2(TIM3, -leftSpeed);         // leftSpeed是负数，取绝对值
		GPIO_ResetBits(GPIOB, GPIO_Pin_6);         // AIN2 = 0
		GPIO_SetBits(GPIOB, GPIO_Pin_5);           // AIN1 = 1
	}
	else  // leftSpeed == 0
	{
		// 刹车：PWM=0, AIN1=1 AIN2=1（短路制动）
		TIM_SetCompare2(TIM3, 0);
		GPIO_SetBits(GPIOB, GPIO_Pin_5 | GPIO_Pin_6);   // 两个同时置1
	}

	// ==================== 右电机 ====================
	// PA6 = TIM3_CH1(PWM)   PB7=BIN1   PB8=BIN2
	// 逻辑和左电机完全一样，只是控制不同引脚

	if (rightSpeed > 0)
	{
		TIM_SetCompare1(TIM3, rightSpeed);
		GPIO_SetBits(GPIOB, GPIO_Pin_7);           // BIN1 = 1
		GPIO_ResetBits(GPIOB, GPIO_Pin_8);         // BIN2 = 0
	}
	else if (rightSpeed < 0)
	{
		TIM_SetCompare1(TIM3, -rightSpeed);
		GPIO_ResetBits(GPIOB, GPIO_Pin_7);         // BIN1 = 0
		GPIO_SetBits(GPIOB, GPIO_Pin_8);           // BIN2 = 1
	}
	else
	{
		TIM_SetCompare1(TIM3, 0);
		GPIO_SetBits(GPIOB, GPIO_Pin_7 | GPIO_Pin_8);   // 刹车
	}
}
