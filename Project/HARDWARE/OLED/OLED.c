#include "oled.h"
#include "olediic.h"
#include "delay.h"

// 显存数组：128列 x 8页 = 128x64像素
static u8 OLED_GRAM[OLED_WIDTH][8];

static void OLED_WriteByte(u8 byte, u8 mode)
{
	I2C_Start();
	I2C_SendByte(0x78);
	I2C_WaitAck();
	if (mode == OLED_DATA)
		I2C_SendByte(0x40);
	else
		I2C_SendByte(0x00);
	I2C_WaitAck();
	I2C_SendByte(byte);
	I2C_WaitAck();
	I2C_Stop();
}

void OLED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_SetBits(GPIOA, GPIO_Pin_11 | GPIO_Pin_12);

	OLED_WriteByte(0xAE, OLED_CMD);
	OLED_WriteByte(0x00, OLED_CMD);
	OLED_WriteByte(0x10, OLED_CMD);
	OLED_WriteByte(0x40, OLED_CMD);
	OLED_WriteByte(0x81, OLED_CMD);
	OLED_WriteByte(0xCF, OLED_CMD);
	OLED_WriteByte(0xA1, OLED_CMD);
	OLED_WriteByte(0xC8, OLED_CMD);
	OLED_WriteByte(0xA6, OLED_CMD);
	OLED_WriteByte(0xA8, OLED_CMD);
	OLED_WriteByte(0x3F, OLED_CMD);
	OLED_WriteByte(0xD3, OLED_CMD);
	OLED_WriteByte(0x00, OLED_CMD);
	OLED_WriteByte(0xD5, OLED_CMD);
	OLED_WriteByte(0x80, OLED_CMD);
	OLED_WriteByte(0xD9, OLED_CMD);
	OLED_WriteByte(0xF1, OLED_CMD);
	OLED_WriteByte(0xDA, OLED_CMD);
	OLED_WriteByte(0x12, OLED_CMD);
	OLED_WriteByte(0xDB, OLED_CMD);
	OLED_WriteByte(0x30, OLED_CMD);
	OLED_WriteByte(0x20, OLED_CMD);
	OLED_WriteByte(0x02, OLED_CMD);
	OLED_WriteByte(0x8D, OLED_CMD);
	OLED_WriteByte(0x14, OLED_CMD);
	OLED_WriteByte(0xAF, OLED_CMD);

	OLED_Clear();
}

void OLED_Clear(void)
{
	u8 x, page;
	for (page = 0; page < 8; page++)
		for (x = 0; x < OLED_WIDTH; x++)
			OLED_GRAM[x][page] = 0x00;
	OLED_Refresh();
}

void OLED_Refresh(void)
{
	u8 x, page;
	for (page = 0; page < 8; page++)
	{
		OLED_WriteByte(0xB0 + page, OLED_CMD);
		OLED_WriteByte(0x00, OLED_CMD);
		OLED_WriteByte(0x10, OLED_CMD);

		I2C_Start();
		I2C_SendByte(0x78);
		I2C_WaitAck();
		I2C_SendByte(0x40);
		I2C_WaitAck();
		for (x = 0; x < OLED_WIDTH; x++)
		{
			I2C_SendByte(OLED_GRAM[x][page]);
			I2C_WaitAck();
		}
		I2C_Stop();
	}
}

void OLED_DrawPoint(u8 x, u8 y, u8 color)
{
	u8 page, bitMask;

	if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;

	page    = y / 8;
	bitMask = 1 << (y % 8);

	if (color == 1)
		OLED_GRAM[x][page] |=  bitMask;
	else
		OLED_GRAM[x][page] &= ~bitMask;
}

void OLED_ShowChar(u8 x, u8 y, char chr, u8 fontSize, u8 mode)
{
	u8 i, j, byteData, charIndex;

	if (chr < ' ' || chr > '~') return;
	charIndex = chr - ' ';

	if (fontSize == 8)
	{
		for (i = 0; i < 6; i++)
		{
			byteData = asc2_0806[charIndex][i];
			for (j = 0; j < 8; j++)
			{
				if (byteData & 0x01)
					OLED_DrawPoint(x + i, y + j, mode ? 0 : 1);
				else
					OLED_DrawPoint(x + i, y + j, mode ? 1 : 0);
				byteData >>= 1;
			}
		}
	}
	else if (fontSize == 16)
	{
		for (i = 0; i < 8; i++)
		{
			byteData = asc2_1608[charIndex][i];
			for (j = 0; j < 8; j++)
			{
				if (byteData & 0x01)
					OLED_DrawPoint(x + i, y + j, mode ? 0 : 1);
				else
					OLED_DrawPoint(x + i, y + j, mode ? 1 : 0);
				byteData >>= 1;
			}
		}
		for (i = 0; i < 8; i++)
		{
			byteData = asc2_1608[charIndex][i + 8];
			for (j = 0; j < 8; j++)
			{
				if (byteData & 0x01)
					OLED_DrawPoint(x + i, y + 8 + j, mode ? 0 : 1);
				else
					OLED_DrawPoint(x + i, y + 8 + j, mode ? 1 : 0);
				byteData >>= 1;
			}
		}
	}
}

void OLED_ShowString(u8 x, u8 y, char *str, u8 fontSize, u8 mode)
{
	u8 posX = x;

	while (*str != '\0')
	{
		OLED_ShowChar(posX, y, *str, fontSize, mode);

		if (fontSize == 8)
			posX += 6;
		else
			posX += 8;

		if (posX >= OLED_WIDTH - 6)
		{
			posX = 0;
			y += fontSize;
		}

		str++;
	}
}

void OLED_ShowNum(u8 x, u8 y, uint32_t num, u8 len, u8 fontSize, u8 mode)
{
	u8 buf[10];
	u8 digits = 0;
	u8 i, step, posX = x;

	step = (fontSize == 8) ? 6 : 8;

	// 分离各位数字
	if (num == 0)
	{
		buf[0] = 0;
		digits = 1;
	}
	else
	{
		while (num > 0)
		{
			buf[digits++] = num % 10;
			num /= 10;
		}
	}

	// 右对齐：前面补空格
	for (i = 0; i < len - digits; i++)
	{
		OLED_ShowChar(posX, y, ' ', fontSize, mode);
		posX += step;
	}

	// 从高位到低位显示
	for (i = digits; i > 0; i--)
	{
		OLED_ShowChar(posX, y, '0' + buf[i - 1], fontSize, mode);
		posX += step;
	}
}
