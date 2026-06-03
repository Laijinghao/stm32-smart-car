/**
 * ============================================================
 * STM32F103C8T6 智能小车 —— 主程序
 *
 * 功能：四路红外循迹 + 超声波避障 + 舵机扫描 + OLED显示 + 蓝牙遥控
 *
 * 【引脚接线一览】
 *   电机TB6612  : PB5(AIN1) PB6(AIN2) PB7(BIN1) PB8(BIN2) — 方向
 *                  PA7(PWMA)  PA6(PWMB)                    — 速度PWM
 *   超声波HCSR04 : PA4(Trig) PA5(Echo)
 *   舵机SG90     : PA0(PWM)
 *   循迹传感器  : PB0(X1) PB1(X2) PB10(X3) PB11(X4)
 *   OLED (I2C)  : PA11(SCL) PA12(SDA)
 *   蓝牙JDY-31  : PA9(TX) PA10(RX)
 *   板载LED     : PC13 (低电平亮)
 *
 * 【程序结构】
 *   main() 分为4个步骤：
 *     第1步：系统初始化（中断优先级分组）
 *     第2步：逐个初始化所有硬件模块（顺序有一定讲究，见下面注释）
 *     第3步：OLED显示开机画面1.5秒
 *     第4步：进入无限循环（不断地：检查蓝牙指令 → 执行控制 → 刷新OLED）
 * ============================================================
 */

#include "stm32f10x.h"
#include "Delay.h"
#include "sys.h"
#include "LED.h"
#include "TIM3_PWM.h"
#include "MOTOR.h"
#include "HCSR04.h"
#include "Servo.h"
#include "Trace.h"
#include "OLED.h"
#include "USART1.h"
#include "control.h"


/**
 * ============================================================
 * OLED_ShowStatus —— 把当前状态画到OLED屏幕上
 *
 * 显示4行内容：
 *   第1行：当前模式（AUTO / TRACE / AVOID / BLUETOOTH）
 *   第2行：前方距离（如 "15cm" 或 "--- cm" 表示超量程）
 *   第3行：4路循迹传感器原始值（如 "T:0 0 1 1"）
 *   第4行：提示文字 "1-4:Switch Mode"
 *
 * 【static 变量的作用 —— 防闪烁】
 *   lastDist 和 lastMode 是静态局部变量：
 *     static = 函数退出后值不丢失，下次进来还是上次的值
 *   每次进来先比较"当前值"和"上次值"：
 *     如果一样 → 说明数据没变，跳过，不刷屏
 *     如果不一样 → 说明有变化，才刷新OLED
 *   这样做的好处：
 *     1. 减少I2C通信次数（I2C很慢，频繁刷屏会让主循环变卡）
 *     2. 屏幕不闪烁（一直刷同样的内容会闪）
 *
 * 【坐标系】
 *   (x=0, y=0) 是屏幕左上角
 *   x向右变大(0~127)，y向下变大(0~63)
 *   OLED_ShowString(x, y, 文字, 字号, 反色)
 *     字号：8=小字(6x8像素)  16=大字(8x16像素)
 *     反色：0=白底黑字(正常)  1=黑底白字(反色)
 *     每行高度 = 字号像素
 *     第0行(y=0)、第1行(y=2)、第2行(y=4)、第3行(y=6)
 * ============================================================
 */
void OLED_ShowStatus(void)
{
	// static变量：第一次调用时初始化为0，之后值保持
	static uint16_t lastDist = 0;   // 上次显示的距离值
	static uint8_t  lastMode = 0;   // 上次显示的模式

	// 只在数据变化时才刷新OLED
	if (obstacleDist != lastDist || sysMode != lastMode)
	{
		lastDist = obstacleDist;   // 更新缓存
		lastMode = sysMode;

		// ===== 第1行：模式 =====
		OLED_ShowString(0, 0, "Mode:", 8, 1);   // "Mode:"用8号小字
		switch (sysMode)
		{
			case MODE_AUTO:      OLED_ShowString(32, 0, "Auto",      8, 1); break;
			case MODE_TRACE:     OLED_ShowString(32, 0, "Trace",     8, 1); break;
			case MODE_AVOID:     OLED_ShowString(32, 0, "Avoid",     8, 1); break;
			case MODE_BLUETOOTH: OLED_ShowString(32, 0, "Bluetooth", 8, 1); break;
			default:             OLED_ShowString(32, 0, "Unknown",   8, 1); break;
		}

		// ===== 第2行：距离 =====
		OLED_ShowString(0, 2, "Dist:", 8, 1);
		if (obstacleDist > 0)
		{
			// obstacleDist > 0 表示测到了有效距离
			OLED_ShowNum(32, 2, obstacleDist, 3, 8, 1);    // 显示3位数字（如 " 15"）
			OLED_ShowString(56, 2, "cm", 8, 1);             // 单位
		}
		else
		{
			// obstacleDist == 0 表示超量程（4米内没东西）
			OLED_ShowString(32, 2, "--- cm", 8, 1);
		}

		// ===== 第3行：循迹传感器原始值 =====
		// 显示4个传感器当前的读数，调试用
		OLED_ShowString(0, 4, "T:", 8, 1);
		OLED_ShowNum(16, 4, TRACE_X1, 1, 8, 1);   // X1：0=踩线 1=白底
		OLED_ShowNum(28, 4, TRACE_X2, 1, 8, 1);   // X2
		OLED_ShowNum(40, 4, TRACE_X3, 1, 8, 1);   // X3
		OLED_ShowNum(52, 4, TRACE_X4, 1, 8, 1);   // X4

		// ===== 第4行：操作提示 =====
		OLED_ShowString(0, 6, "1-4:Switch Mode", 8, 1);

		// 把显存里的内容推到屏幕上（不调用这行屏幕不会更新！）
		OLED_Refresh();
	}
}


/**
 * ============================================================
 * main() —— 程序入口，单片机一上电就从这里开始执行
 *
 * 【初始化顺序为什么这样排？】
 *   原则：先初始化"被依赖的"模块，再初始化"依赖别人的"模块。
 *   但这里其实没有强依赖，所以顺序影响不大。
 *   唯一的讲究：
 *     1. LED最先初始化 → 初始化出错时能闪灯提示
 *     2. TIM3_PWM 先于 Motor_GPIO  → 电机方向引脚要有PWM信号配合
 *     3. OLED最后初始化 → 前面的模块初始化过程中如果有问题，
 *        可以通过LED闪烁来提示（因为OLED还没准备好）
 *     4. Control_Init 放最后 → 它调用了Servo和Delay，这两个必须已初始化
 *
 * 【主循环的结构 —— 三种任务轮流执行】
 *   while(1) 是个死循环，每轮做3件事：
 *     A. 检查蓝牙有没有发模式切换指令（'1'~'4'）
 *        → 有的话切换模式 + 闪灯提示
 *     B. 执行当前模式的逻辑（自动/循迹/避障/蓝牙）
 *        → 具体逻辑见 control.c
 *     C. 刷新OLED显示
 *        → 只在数据变化时才刷新（防闪烁）
 *
 * 【为什么模式切换不在 control.c 里处理？】
 *   模式切换需要"在任何模式下都能触发"，
 *   放在主循环最前面可以保证无论当前在哪个模式，
 *   优先处理模式切换，然后再执行当前模式的逻辑。
 *   就像手机：无论在哪个APP里，按电源键都能锁屏（最高优先级）。
 * ============================================================
 */
int main(void)
{
	// ========== 第1步：系统级配置 ==========

	// 中断优先级分组：2位抢占 + 2位子响应
	// 抢占优先级数字越小越优先（0最高）
	// 抢占相同的情况下才比子优先级
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);


	// ========== 第2步：逐个初始化硬件 ==========

	// LED最先初始化——后续如果某个模块初始化失败，至少能闪灯报警
	LED_Init();
	LED_ON();    // 初始化中，LED亮着表示"工作中"

	// 电机PWM：arr=7199 psc=1 → PWM频率=72MHz/(2×7200)=5KHz
	// 选5KHz是因为人耳听不到这个频率（不刺耳），电机响应也够快
	TIM3_PWM_Init(7199, 1);

	// 电机方向引脚（PB5~PB8 = TB6612的IN1/IN2）
	Motor_GPIO_Init();

	// 四路循迹传感器（PB0 PB1 PB10 PB11 = 上拉输入）
	Trace_Init();

	// 超声波HC-SR04（PA4=Trig PA5=Echo + TIM4计时器）
	HCSR04_Init();

	// 舵机SG90：arr=1999 psc=719 → 50Hz
	// 舵机信号必须是50Hz（周期20ms），这是SG90的硬件要求
	Servo_Init(1999, 719);

	// OLED屏幕（PA11=SCL PA12=SDA 软件I2C）
	OLED_Init();
	OLED_Clear();   // 清屏，准备好显示

	// 蓝牙串口（PA9=TX PA10=RX 波特率9600）
	// 9600是JDY-31蓝牙模块的默认波特率，改不了
	USART1_Init(9600);

	// 控制系统（舵机归位到90度）
	Control_Init();

	LED_OFF();   // 所有模块初始化完成，灭灯表示"就绪"


	// ========== 第3步：开机画面（显示1.5秒） ==========
	OLED_ShowString(10, 0, "Smart Car", 16, 1);    // 大号字标题
	OLED_ShowString(10, 3, "STM32F103", 8, 1);     // 芯片型号
	OLED_ShowString(10, 4, "Trace+Avoid", 8, 1);   // 功能介绍
	OLED_ShowString(10, 6, "Init OK!", 8, 1);      // 初始化成功
	OLED_Refresh();
	Delay_ms(1500);    // 停留1.5秒让人看清楚
	OLED_Clear();      // 清屏，准备显示实时数据


	// ========== 第4步：主循环（永远不会退出） ==========
	while (1)
	{
		// --- A. 检查蓝牙模式切换指令（优先级最高） ---
		// RxData 是 USART1中断自动更新的全局变量
		// '1'~'4' 的ASCII码对应 0x31~0x34
		if (RxData >= '1' && RxData <= '4')
		{
			sysMode = RxData - '0';   // '1' - '0' = 1, '2' - '0' = 2 ...
			RxData = 0;               // 清零！防止下次循环又切换一次
			LED_Flash();              // 快闪两次表示"收到切换指令"
		}

		// --- B. 根据当前模式执行控制逻辑 ---
		switch (sysMode)
		{
			case MODE_AUTO:
				Control_AutoMode();      // 自动：循迹+避障
				break;

			case MODE_TRACE:
				Control_TraceMode();     // 纯循迹
				break;

			case MODE_AVOID:
				Control_AvoidMode();     // 纯避障
				break;

			case MODE_BLUETOOTH:
				Control_BluetoothMode(); // 蓝牙遥控
				break;

			default:
				// 非法模式值 → 回退到安全的自动模式
				sysMode = MODE_AUTO;
				break;
		}

		// --- C. 刷新OLED（内部会自动判断是否需要刷） ---
		OLED_ShowStatus();

		// 一轮循环结束，又回到 while(1) 开头，周而复始
	}
}
