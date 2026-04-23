#pragma once
// kys_littlegame.h - 小游戏声明
// 对应 kys_littlegame.pas interface

#include "kys_type.h"
#include <SDL3/SDL.h>

// 小游戏公开函数
bool Acupuncture(int n);
bool ShotEagle(int aim, int chance);
bool Poetry(int talknum, int chance, int c, int Count);
bool Lamp(int c, int beginpic, int whitecount, int chance);
bool rotoSpellPicture(int num, int chance);
int  FemaleSnake();
int  movesnake(int Edest);
void randomsnake();
int  iif(bool cond, int TrueReturn, int FalseReturn);

// 模块内部变量
inline TPic background_{}, snakepic_{}, Weipic_{};
inline SDL_Surface* femalepic_[8]{};
inline int EatFemale = 0;

// dest, RANX, RANY, snake 已在 kys_type.h 中定义

// 通用游戏数组 (Poetry / Lamp / rotoSpellPicture 共用)
inline std::vector<std::vector<int16_t>> gamearray;