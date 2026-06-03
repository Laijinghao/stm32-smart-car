#ifndef __SYS_H
#define __SYS_H
#include "stm32f10x.h"

// ============================================================
// 系统公共头文件
// 功能：定义常用数据类型别名 & 位带操作宏(类似51单片机风格操作IO口)
// ============================================================

// --- 常用数据类型缩写(方便跨平台) ---
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

// --- 位带操作: 实现像51单片机那样直接操作单个IO口 ---
// 原理：STM32的位带区将每个bit映射到一个独立的32位地址
#define BITBAND(addr, bitnum)  ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
#define MEM_ADDR(addr)         *((volatile unsigned long *)(addr))
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND(addr, bitnum))

// --- 各GPIO端口输出寄存器地址 ---
#define GPIOA_ODR_Addr  (GPIOA_BASE + 12)
#define GPIOB_ODR_Addr  (GPIOB_BASE + 12)
#define GPIOC_ODR_Addr  (GPIOC_BASE + 12)

// --- 各GPIO端口输入寄存器地址 ---
#define GPIOA_IDR_Addr  (GPIOA_BASE + 8)
#define GPIOB_IDR_Addr  (GPIOB_BASE + 8)
#define GPIOC_IDR_Addr  (GPIOC_BASE + 8)

// --- 位带操作宏：PAout(0) = 1 就等价于让PA0输出高电平 ---
#define PAout(n)  BIT_ADDR(GPIOA_ODR_Addr, n)  // PA端口输出
#define PAin(n)   BIT_ADDR(GPIOA_IDR_Addr, n)  // PA端口输入
#define PBout(n)  BIT_ADDR(GPIOB_ODR_Addr, n)  // PB端口输出
#define PBin(n)   BIT_ADDR(GPIOB_IDR_Addr, n)  // PB端口输入
#define PCout(n)  BIT_ADDR(GPIOC_ODR_Addr, n)  // PC端口输出
#define PCin(n)   BIT_ADDR(GPIOC_IDR_Addr, n)  // PC端口输入

#endif
