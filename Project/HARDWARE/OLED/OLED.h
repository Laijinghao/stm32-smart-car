#ifndef __OLED_H
#define __OLED_H
#include "sys.h"

#define OLED_WIDTH   128
#define OLED_HEIGHT   64

#define OLED_CMD   0
#define OLED_DATA  1

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_DrawPoint(u8 x, u8 y, u8 color);
void OLED_ShowChar(u8 x, u8 y, char chr, u8 fontSize, u8 mode);
void OLED_ShowString(u8 x, u8 y, char *str, u8 fontSize, u8 mode);
void OLED_ShowNum(u8 x, u8 y, uint32_t num, u8 len, u8 fontSize, u8 mode);

extern const unsigned char asc2_0806[95][6];
extern const unsigned char asc2_1608[95][16];

#endif
