/**
 * ============================================================
 * SG90 舵机驱动
 *
 * 【舵机是什么？】
 *   舵机不是普通的电机。普通电机你通电它就转，断电就停。
 *   舵机是"指哪转哪"：你告诉它"转到90度"，它就精确转到90度停住。
 *   就像汽车的转向盘，有角度概念。
 *
 * 【它是怎么工作的？】
 *   舵机内部：一个小电机 + 齿轮组 + 电位器(角度传感器) + 控制电路
 *   工作流程：
 *     1. 你给它一个PWM信号（高电平持续时间表示目标角度）
 *     2. 内部控制电路读取PWM信号 → 得到目标角度
 *     3. 读电位器 → 得到当前角度
 *     4. 比较两个角度 → 驱动小电机转动 → 直到两个角度一致
 *
 * 【PWM信号格式 —— 这是理解舵机的关键！】
 *   PWM频率必须是50Hz（周期固定为20ms，改不了！）
 *   高电平的持续时间表示目标角度：
 *
 *     高电平 0.5ms  →  角度 0度    (最左)
 *     高电平 1.0ms  →  角度 45度
 *     高电平 1.5ms  →  角度 90度   (正中)
 *     高电平 2.0ms  →  角度 135度
 *     高电平 2.5ms  →  角度 180度  (最右)
 *
 *   【为什么是50Hz？】
 *    这是SG90的硬件设计要求。舵机内部电路每20ms检查一次PWM信号，
 *    然后调整位置。如果频率不对，舵机会抖动或者不工作。
 *
 * 【CCR值和角度的对应关系】
 *   TIM2的配置：arr=1999, psc=719, 72MHz时钟
 *   定时器频率 = 72MHz / (psc+1) = 72MHz / 720 = 100KHz
 *   PWM周期   = (arr+1) / 100KHz = 2000 / 100KHz = 20ms → 50Hz ✓
 *
 *   CCR值和脉宽的换算：每个CCR = 1/100KHz = 0.01ms = 10us
 *     高电平0.5ms → CCR = 0.5ms / 0.01ms = 50
 *     高电平1.5ms → CCR = 1.5ms / 0.01ms = 150
 *     高电平2.5ms → CCR = 2.5ms / 0.01ms = 250
 *
 *   角度→CCR的公式（线性插值）：
 *     CCR = 50 + (200 × angle) / 180
 *     验证：angle=0  → CCR=50  ✓
 *           angle=90 → CCR=50+100=150 ✓
 *           angle=180→ CCR=50+200=250 ✓
 * ============================================================
 */

#include "Servo.h"
#include "Delay.h"

// 舵机的CCR范围：0度=50, 180度=250
#define SERVO_CCR_MIN   50
#define SERVO_CCR_MAX   250


/**
 * Servo_Init —— 初始化舵机PWM
 *
 * 用TIM2的通道1（PA0引脚）输出50Hz的PWM波给舵机。
 *
 * @param arr : 自动重装载值 → 决定PWM周期
 * @param psc : 预分频值     → 决定定时器计数频率
 *
 * 【参数怎么选？】
 *   目标是50Hz（20ms周期）。
 *   定时器频率 = 72MHz / (psc+1)
 *   PWM频率   = 定时器频率 / (arr+1)
 *
 *   选 psc=719, arr=1999：
 *     定时器频率 = 72MHz / 720 = 100KHz = 0.01ms/tick
 *     PWM频率   = 100KHz / 2000 = 50Hz ✓
 *     CCR每+1 = 高电平增加0.01ms，精度够了
 */
void Servo_Init(u16 arr, u16 psc)
{
	// --- 开启时钟 ---
	// TIM2在APB1总线，GPIOA在APB2总线
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,  ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// --- 配置PA0为"复用推挽输出" ---
	// 复用 = 引脚的控制权交给定时器（不是普通GPIO直接控制）
	// 推挽 = 能主动输出高低电平
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;        // PA0
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;   // AF=Alternate Function复用
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// --- 时基配置：决定PWM频率 ---
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period        = arr;    // ARR：计数到arr就归零
	TIM_TimeBaseStructure.TIM_Prescaler     = psc;    // PSC：对72MHz先分个频
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;      // 数字滤波器用，填0就行
	TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up; // 向上计数
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// --- 通道1的PWM模式配置 ---
	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode      = TIM_OCMode_PWM1;
	// PWM模式1的规则：计数值 < CCR → 输出高电平    计数值 > CCR → 输出低电平
	// 所以CCR值 = 高电平持续的计数值
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 使能输出
	TIM_OCInitStructure.TIM_OCPolarity  = TIM_OCPolarity_High;    // 高电平有效

	TIM_OC1Init(TIM2, &TIM_OCInitStructure);                       // 初始化CH1
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
	// 预装载 = 写到CCR的值不会立刻生效，等到下一次定时器溢出才更新
	// 好处：不会在PWM周期中途改变占空比，信号更干净

	// --- 启动定时器 ---
	TIM_Cmd(TIM2, ENABLE);
}


/**
 * Servo_SetAngle —— 让舵机转到指定角度
 *
 * @param angle : 0~180度，超出范围自动限幅到180
 *
 * 【Delay_ms(300) 为什么要等300ms？】
 *   SG90的转速约 0.12秒/60度 = 0.002秒/度
 *   最坏情况从0度转到180度 = 180 × 0.002 = 0.36秒 ≈ 360ms
 *   这里取300ms是折中：大多数情况够用，不会等太久。
 *
 *   【注意】这个延迟会阻塞CPU 300ms！在这个300ms内
 *   小车什么都不能做（不能测距、不能循迹、不能响应蓝牙）。
 *   这是简化设计的代价。正规项目中会用非阻塞的方式。
 */
void Servo_SetAngle(uint8_t angle)
{
	// 限幅：舵机最大180度，超过就截断
	if (angle > 180) angle = 180;

	// 角度 → CCR线性映射
	// CCR = 50(最小值) + (200 × angle) / 180
	// 比方angle=90: CCR = 50 + 200×90/180 = 50 + 100 = 150
	uint16_t pulse = SERVO_CCR_MIN + ((uint32_t)angle * (SERVO_CCR_MAX - SERVO_CCR_MIN)) / 180;

	// 更新比较寄存器，改变PWM占空比
	TIM_SetCompare1(TIM2, pulse);

	// 等舵机转到目标位置（物理转动需要时间）
	Delay_ms(300);
}
