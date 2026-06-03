/**
 * ============================================================
 * HC-SR04 超声波测距模块驱动
 *
 * 【HC-SR04是什么？】
 *   一个像两个眼睛的小电路板，可以测量前方物体的距离。
 *   原理和蝙蝠回声定位一模一样：
 *     蝙蝠发出超声波 → 声波碰到障碍物弹回来 → 蝙蝠听到回声
 *     → 根据"发出到听到"的时间差 × 声速 ÷ 2 = 距离
 *
 * 【它是怎么和STM32通信的？】
 *   HC-SR04有4个引脚：
 *     VCC(5V)  GND(地)  Trig(触发)  Echo(回声)
 *
 *   STM32通过Trig告诉HC-SR04"开始测"：
 *     STM32拉高Trig → 保持至少10us → 拉低
 *     HC-SR04看到这个脉冲后，自动发出8个40KHz的超声波
 *
 *   HC-SR04通过Echo告诉STM32"测完了"：
 *     超声波发出后，Echo自动变成高电平
 *     等超声波反射回来被接收到，Echo变回低电平
 *     Echo高电平持续的时间 = 超声波在天上飞的时间
 *
 * 【距离怎么算？】
 *   声速 = 340m/s = 34000cm/s
 *   假设Echo高电平持续了 T 秒：
 *     超声波飞的路程 = 34000 × T （厘米）
 *     但这是"去+回"的总路程！实际障碍物距离只需要一半
 *     障碍物距离 = (34000 × T) ÷ 2
 *
 * 【本程序的具体做法】
 *   用TIM4定时器来"数"Echo高电平持续了多久：
 *     TIM4配置：72MHz时钟，每个tick = 1/72000000 秒
 *     但每次溢出(ARR=7199)才进一次中断，所以：
 *       一个溢出周期 = 7200 × (1/72000000) = 0.0001秒 = 100us
 *     在中断里检查Echo是不是高电平，是的话 echoTickCount++
 *     最终：echoTickCount = Echo高电平持续了多少个"100us"
 *
 *   距离公式：
 *     时间 = echoTickCount × 0.0001 秒
 *     距离 = (34000 × 时间) ÷ 2
 *          = (34000 × echoTickCount × 0.0001) ÷ 2
 *          = echoTickCount × 1.7
 *     （在代码里直接用浮点算，编译器会处理）
 *
 * 【TIM4为什么ARR=7199？】
 *   72MHz / (PSC+1) / (ARR+1) = 72MHz / 1 / 7200 = 10KHz
 *   每个溢出周期 = 1/10000 = 100us
 *   即每100us中断一次，精度足够（1.7cm分辨率）
 * ============================================================
 */

#include "stm32f10x.h"
#include "HCSR04.h"
#include "delay.h"

// ==================== 全局变量 ====================
// volatile 告诉编译器：这个变量的值可能在任何时候被中断改变
// 不要对它做优化（比如缓存到寄存器里），每次用都要从内存重新读
static volatile uint16_t echoTickCount = 0;
// echoTickCount 每累加1 = Echo高电平持续了100us


/**
 * ============================================================
 * HCSR04_Init —— 初始化超声波模块
 *
 * 做两件事：
 *   1. 配置GPIO：Trig(PA4)=推挽输出  Echo(PA5)=下拉输入
 *   2. 配置TIM4：作为Echo高电平的计时器，每100us中断一次
 *
 * 【GPIO配置解释】
 *   Trig → 推挽输出：STM32主动发出高低电平，驱动HC-SR04
 *   Echo → 下拉输入：默认通过内部电阻拉到GND(低电平)
 *          当HC-SR04输出高电平时，PA5读到高电平
 *          不拉低的话，悬空时电平不确定，会测出乱七八糟的值
 *
 *   TIM4放在这里初始化（只初始化一次），
 *   而不是每次测距都重新初始化。以前的老代码每次测距都重新
 *   初始化一遍定时器，浪费CPU时间，也没必要。
 * ============================================================
 */
void HCSR04_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	// ----- GPIO：Trig(PA4)=推挽输出  Echo(PA5)=下拉输入 -----
	// 使能GPIOA的时钟（STM32的每个外设用之前都要先开时钟，否则不工作）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// 配置PA4为推挽输出（输出能力强，能驱动超声波模块）
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;   // PP = Push-Pull 推挽
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// 配置PA5为下拉输入（没人给它信号时默认是低电平）
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPD;      // IPD = Input Pull-Down
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_5;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// 初始化时拉低Trig（不发信号）
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);

	// ----- TIM4：每100us溢出一次的计时器 -----
	// TIM4挂在APB1总线上，所以要开APB1的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	// 选择内部时钟源（72MHz的系统时钟）
	TIM_InternalClockConfig(TIM4);

	// 时基配置：决定定时器多长时间溢出一次
	// 溢出时间 = (PSC+1) × (ARR+1) / 时钟频率
	//          = (0+1) × (7199+1) / 72000000
	//          = 7200 / 72000000
	//          = 0.0001秒 = 100微秒
	TIM_TimeBaseInitStruct.TIM_ClockDivision   = TIM_CKD_DIV1;     // 输入时钟不分频
	TIM_TimeBaseInitStruct.TIM_CounterMode     = TIM_CounterMode_Up; // 向上计数：0→1→...→ARR→0
	TIM_TimeBaseInitStruct.TIM_Period          = 7199;               // ARR自动重装载值
	TIM_TimeBaseInitStruct.TIM_Prescaler       = 0;                  // PSC预分频：不分频=72MHz
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStruct);

	// 清除可能残留的更新标志，然后使能更新中断
	TIM_ClearFlag(TIM4, TIM_FLAG_Update);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	// 每当计数器从ARR溢出到0时，触发TIM4_IRQHandler中断

	// 配置TIM4中断的优先级
	NVIC_InitStruct.NVIC_IRQChannel                   = TIM4_IRQn;      // TIM4的中断通道号
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;         // 使能
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2;              // 抢占优先级2(中等)
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 1;              // 子优先级1
	NVIC_Init(&NVIC_InitStruct);

	// 注意：这里不启动定时器！启动放在 ReadDistance() 里。
	// 因为平时不需要计时，只有测距的时候才启动，省电、减少不必要的中断。
}


/**
 * ============================================================
 * TIM4_IRQHandler —— TIM4的中断服务函数
 *
 * TIM4每100us溢出一次，就会自动跳到这个函数。
 * 在这个函数里检查Echo(PA5)的电平：
 *   如果Echo是高电平 → echoTickCount++
 *   如果Echo是低电平 → 什么都不做
 *
 * 所以测量结束后，echoTickCount = Echo高电平持续的"100us个数"
 *
 * 【中断是怎么工作的？(简单理解)】
 *   STM32在执行主循环的某个地方 → TIM4溢出 → 硬件暂停主程序
 *   → 跳转到TIM4_IRQHandler() → 执行完这个函数 → 回到主程序刚才被暂停的地方
 *   整个过程对主程序是"透明"的（主程序不知道被打断了）
 *
 * 【TIM_GetITStatus 和 TIM_ClearITPendingBit 是什么意思？】
 *   - TIM_GetITStatus：查询"是哪个中断源触发了我？"
 *     这里只可能有一个来源（TIM_IT_Update），但必须判断，
 *     因为多个中断可能共享一个IRQHandler
 *   - TIM_ClearITPendingBit：清除中断标志位
 *     如果不清除，退出函数后硬件会立即再次触发中断，造成死循环
 *     （这是STM32初学者最容易忘记的操作！）
 * ============================================================
 */
void TIM4_IRQHandler(void)
{
	// 确认是TIM4的"更新中断"（计数器溢出）
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		// 读一下PA5（Echo引脚）的电平
		// 如果是高电平（=1），说明超声波还在天上飞，计数器+1
		if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5) == 1)
		{
			echoTickCount++;
		}

		// 【关键】清除中断标志！不然后果是：退出这个函数 → 立即再进 → 死循环
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}


/**
 * ============================================================
 * HCSR04_ReadDistance —— 测一次距离
 *
 * 【调用一次这个函数的完整时间线】
 *
 *   时间点          发生了什么                    echoTickCount
 *   ──────────────────────────────────────────────────────────
 *   0ms       echoTickCount清零，TIM4启动        0
 *   0ms       拉高Trig                            0
 *   45us      拉低Trig（触发脉冲结束）             0
 *   ~200us    HC-SR04发出8个40KHz超声波           0
 *   ~300us    Echo变高电平                        开始增长
 *   ...       超声波在天上飞                      ...增长中
 *   ~Xms      超声波碰到障碍物反射回来
 *   ~Xms      Echo变低电平                        停止增长 = 最终值
 *   100ms     TIM4关闭，计算距离                  最终值
 *
 *   为什么等100ms这么久？
 *   HC-SR04手册建议两次测量之间至少间隔60ms，
 *   这是为了防止上一次的超声波还有残余回响。
 *   用100ms是留了安全裕量。
 *
 * 【返回值说明】
 *   返回 0      = 超量程(>4m)或者没测到东西
 *   返回 1~400  = 障碍物距离，单位厘米
 *
 * 【超量程判断逻辑】
 *   4米 = 400cm
 *   400cm对应的echoTickCount = 400 / 1.7 ≈ 235
 *   所以 echoTickCount > 235 就认为超量程了，返回0
 * ============================================================
 */
uint16_t HCSR04_ReadDistance(void)
{
	// ===== 第1步：准备发射 =====
	echoTickCount = 0;                 // 计数器归零（上一次的值要清掉）
	TIM_SetCounter(TIM4, 0);          // 定时器计数值归零
	TIM_Cmd(TIM4, ENABLE);            // 启动TIM4（开始每100us中断一次）

	// ===== 第2步：发射触发脉冲 =====
	// HC-SR04手册要求Trig高电平至少10us，给45us更保险
	GPIO_SetBits(GPIOA, GPIO_Pin_4);  // Trig = 高电平
	Delay_us(45);                     // 保持 45 微秒
	GPIO_ResetBits(GPIOA, GPIO_Pin_4); // Trig = 低电平（触发完成）

	// ===== 第3步：等待测量完成 =====
	// 这100ms内，TIM4的中断在后台默默地数着Echo高电平的持续时间
	Delay_ms(100);

	// ===== 第4步：测量完毕，关闭定时器 =====
	TIM_Cmd(TIM4, DISABLE);           // TIM4停止，不再产生中断

	// ===== 第5步：超量程检查 =====
	// 235个tick ≈ 400cm（HC-SR04的理论最大量程）
	if (echoTickCount > 235)
	{
		echoTickCount = 0;            // 超量程 → 当"没测到"处理
	}

	// ===== 第6步：计算距离 =====
	// 距离 = (tick数 × 0.0001秒/tick × 34000厘米/秒) ÷ 2(来回)
	// 直接用浮点计算，编译器会处理好
	return ((echoTickCount * 0.0001f) * 34000) / 2;
}
