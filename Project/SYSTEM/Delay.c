#include "stm32f10x.h"

/**
 * ============================================================
 * 延时函数实现（基于Cortex-M3内核的SysTick定时器）
 * 系统时钟配置为72MHz，SysTick也是72MHz
 * ============================================================
 */

/**
 * @brief  微秒级延时
 * @param  xus : 要延时的微秒数（最大约233015us，即233ms）
 * @note   原理：往SysTick重装载寄存器写入72*xus
 *         72MHz时钟下，每个时钟周期=1/72us，72个周期=1us
 *         启动定时器后轮询COUNTFLAG标志位等待计满
 */
void Delay_us(uint32_t xus)
{
	SysTick->LOAD = 72 * xus;               // 设置重装载值：每微秒72个时钟
	SysTick->VAL  = 0x00;                   // 清空当前计数器
	SysTick->CTRL = 0x00000005;             // 时钟源=HCLK(72MHz), 使能定时器
	while(!(SysTick->CTRL & 0x00010000));   // 等待计数到0（COUNTFLAG置位）
	SysTick->CTRL = 0x00000004;             // 关闭定时器
}

/**
 * @brief  毫秒级延时
 * @param  xms : 要延时的毫秒数
 * @note   循环调用Delay_us(1000)实现
 */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);   // 1ms = 1000us
	}
}

/**
 * @brief  秒级延时
 * @param  xs : 要延时的秒数
 */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);   // 1s = 1000ms
	}
}
