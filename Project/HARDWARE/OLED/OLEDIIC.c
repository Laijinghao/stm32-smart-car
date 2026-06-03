#include "delay.h"
#include "olediic.h"

/**
 * ============================================================
 * 软件模拟I2C时序
 *
 * I2C协议的4个关键信号：
 *   1. 起始信号：SCL高电平时，SDA从高变低 → "通信开始"
 *   2. 停止信号：SCL高电平时，SDA从低变高 → "通信结束"
 *   3. 应答信号：主机发完一字节后释放SDA，从机把SDA拉低表示收到
 *   4. 数据位  ：SCL低电平时SDA可以变化，SCL上升沿时从机读走SDA的电平
 * ============================================================
 */

/**
 * @brief  I2C延时（微秒级小延时，保证时序满足OLED的速度要求）
 */
static void I2C_Delay(void)
{
	u8 i = 5;           // 循环次数，调节时序速度
	while (i--);
}

/**
 * @brief  发送I2C起始信号
 * @note   SCL高电平时，SDA产生一个下降沿（高→低）
 */
void I2C_Start(void)
{
	OLED_SDA_HIGH();      // 先把SDA拉高
	OLED_SCL_HIGH();      // SCL也拉高
	I2C_Delay();          // 延时，满足建立时间
	OLED_SDA_LOW();       // SCL高电平时SDA变低 → 起始信号
	I2C_Delay();
	OLED_SCL_LOW();       // 把SCL拉低，准备传输数据
	I2C_Delay();
}

/**
 * @brief  发送I2C停止信号
 * @note   SCL高电平时，SDA产生一个上升沿（低→高）
 */
void I2C_Stop(void)
{
	OLED_SDA_LOW();       // 先把SDA拉低
	OLED_SCL_HIGH();      // SCL拉高
	I2C_Delay();
	OLED_SDA_HIGH();      // SCL高电平时SDA变高 → 停止信号
	I2C_Delay();
}

/**
 * @brief  等待从机（OLED）的应答信号
 * @note   主机释放SDA后，OLED会在第9个时钟周期把SDA拉低表示"收到"
 */
void I2C_WaitAck(void)
{
	OLED_SDA_HIGH();      // 主机释放SDA线（上拉电阻拉到高电平）
	I2C_Delay();
	OLED_SCL_HIGH();      // 发出第9个时钟脉冲
	I2C_Delay();
	OLED_SCL_LOW();       // 恢复SCL为低
	I2C_Delay();
}

/**
 * @brief  发送一个字节（8位）数据
 * @param  dat : 要发送的字节
 * @note   从最高位(bit7)到最低位(bit0)依次发送
 *         在每个SCL低电平期间放好数据，上升沿时从机读取
 */
void I2C_SendByte(u8 dat)
{
	u8 i;
	for (i = 0; i < 8; i++)           // 循环8次，发送8位
	{
		if (dat & 0x80)                 // 判断当前最高位是1还是0
		{
			OLED_SDA_HIGH();            // 最高位是1 → SDA输出高
		}
		else
		{
			OLED_SDA_LOW();             // 最高位是0 → SDA输出低
		}
		I2C_Delay();
		OLED_SCL_HIGH();                // SCL上升沿：从机读走SDA上的数据
		I2C_Delay();
		OLED_SCL_LOW();                 // SCL下降沿：准备下一位数据
		dat <<= 1;                      // 左移一位，把下一位移到最高位
	}
}
