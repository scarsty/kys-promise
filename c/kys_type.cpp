// kys_type.cpp - 大数组的定义 (extern 声明在 kys_type.h)

#include "kys_type.h"

// 主地图数据
int16_t Earth[480][480] = {};
int16_t Surface[480][480] = {};
int16_t Building[480][480] = {};
int16_t BuildX[480][480] = {};
int16_t BuildY[480][480] = {};
int16_t Entrance[480][480] = {};

// 场景图形映像
uint32_t SceneImg[2304][1152] = {};
uint8_t MaskArray[2304][1152] = {};

// 战场图形映像
uint32_t BFieldImg[2304][1152] = {};

// 战场数据
int16_t BField[8][64][64] = {};

// 扩展指令变量
int16_t x50[0x10000] = {};

// 寻路
int16_t FWay[480][480] = {};
int16_t linex[480 * 480] = {};
int16_t liney[480 * 480] = {};
