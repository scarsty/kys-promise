// kys_battle.h - 战斗系统头文件
// 对应 kys_battle.pas interface
#pragma once

#include "kys_type.h"
#include <string>

// 主入口
bool Battle(int battlenum, int getexp);
bool InitialBField();
int  SelectTeamMembers();
void ShowMultiMenu(int max, int menu, int status);

// 战斗主循环
void BattleMainControl();
void OldBattleMainControl();
int  CountProgress();
void CalMoveAbility();
void ReArrangeBRole();
int  BattleStatus();
void showprogress();

// 菜单
int  BattleMenu(int bnum);
void ShowBMenu(int MenuStatus, int menu, int max);
void ShowBMenu2(int MenuStatus, int menu, int max, int bnum);

// 移动
void MoveRole(int bnum);
void MoveAmination(int bnum);
void collect(int bnum);

// 选择目标
bool Selectshowstatus(int bnum);
bool SelectAim(int bnum, int step);
bool SelectRange(int bnum, int AttAreaType, int step, int range);
bool SelectDirector(int bnum, int AttAreaType, int step, int range);
bool SelectCross(int bnum, int AttAreaType, int step, int range);
bool SelectFar(int bnum, int mnum, int level);
bool SelectLine(int bnum, int AttAreaType, int step, int range);

// 路径
void SeekPath(int x, int y, int step);
void SeekPath2(int x, int y, int step, int myteam, int mode);
void CalCanSelect(int bnum, int mode, int step);

// 攻击 & 武功
void Attack(int bnum);
void AttackAction(int bnum, int mnum, int level);
void ShowMagicName(int mnum);
int  SelectMagic(int bnum);
void ShowMagicMenu(int bnum, int menustatus, int menu, int max);

// 动画 & 范围
void SetAminationPosition(int mode, int step, int range);
void PlayMagicAmination(int bnum, int bigami, int enum_, int level);
void PlayActionAmination(int bnum, int mode);

// 伤害
void CalHurtRole(int bnum, int mnum, int level);
int  CalHurtValue(int bnum1, int bnum2, int mnum, int level);
int  CalNewHurtValue(int lv, int min_, int max_, int Proportion);
int16_t ReMoveHurt(int bnum, int AttackBnum);
int16_t RetortHurt(int bnum, int AttackBnum);
void ShowHurtValue(int mode);
void ShowHurtValue(int sign, int color1, int color2);
void ShowHurtValue(const std::wstring& str, int color1, int color2);

// 状态
void CalPoiHurtLife(int bnum);
void ClearDeadRolePic();
void ShowSimpleStatus(int rnum, int x, int y);
void Wait(int bnum);
void RestoreRoleStatus();
void Rest(int bnum);

// 经验 & 升级
void AddExp();
void CheckLevelUp();
void LevelUp(int bnum);
void CheckBook();
int  CalRNum(int team);

// 物品 & 技能
void BattleMenuItem(int bnum);
void UsePoision(int bnum);
void Medcine(int bnum);
void MedFrozen(int bnum);
void MedPoision(int bnum);
void UseHiddenWeapen(int bnum, int inum);

// 自动战斗
void AutoBattle(int bnum);
void AutoUseItem(int bnum, int list);
void PetEffect();
void AutoBattle2(int bnum);
void Auto(int bnum);

// AI 寻路/攻击评估
void trymoveattack(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int bnum, int mnum, int level);
void calline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void calarea(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void calNewline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void calpoint(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void calcross(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void caldirdiamond(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void caldirangle(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void calfar(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level);
void nearestmove(int& Mx, int& My, int bnum);
void farthestmove(int& Mx, int& My, int bnum);
void trymovecure(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum);
void cureaction(int bnum);
void trymoveHidden(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum, int inum);
void Hiddenaction(int bnum, int inum);
void trymoveUsePoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum);
void UsePoiaction(int bnum);
void trymoveMedPoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum);
void MedPoiaction(int bnum);

// 模式选择
void ShowModeMenu(int menu);
int  SelectAutoMode();
void ShowTeamModeMenu(int menu);
bool TeamModeMenu();