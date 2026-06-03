#ifndef __CONTROL_H
#define __CONTROL_H
#include "sys.h"

// 工作模式
#define MODE_AUTO       1
#define MODE_TRACE      2
#define MODE_AVOID      3
#define MODE_BLUETOOTH  4

// 避障阈值（厘米）
#define OBSTACLE_DIST   25
#define SAFE_DIST       35

// 速度预设
#define SPEED_FAST      5000
#define SPEED_NORMAL    3500
#define SPEED_SLOW      2000
#define SPEED_TURN      2800
#define SPEED_STOP      0

// 全局变量
extern uint8_t  sysMode;
extern uint16_t obstacleDist;
extern uint8_t  bluetoothCmd;

// 控制函数
void Control_Init(void);
void Control_AutoMode(void);
void Control_TraceMode(void);
void Control_AvoidMode(void);
void Control_BluetoothMode(void);
void Car_AvoidObstacle(void);

#endif
