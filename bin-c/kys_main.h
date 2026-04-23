#pragma once
// kys_main.h - 主程序：初始化、行走、菜单、存储
// 对应 kys_main.pas 的 interface 段

#include "kys_type.h"

#include <string>

// ---- 程序入口 ----
int Run(int argc, char* argv[]);
void Quit();

// ---- 游戏流程 ----
void Start();
void StartAmi();
void ReadFiles();
bool InitialRole();
void LoadR(int num);
void SaveR(int num);
int WaitAnyKey();
void WaitAnyKey(int16_t* keycode, int16_t* x, int16_t* y);
void Walk();
bool CanWalk(int x, int y);
bool CheckEntrance();
int WalkInScene(int Open);
void ShowSceneName(int snum);
bool CanWalkInScene(int x, int y);
bool CanWalkInScene(int x1, int y1, int x, int y);
void CheckEvent3();
void ShowRandomAttribute(bool ran);
bool RandomAttribute();
void ReSetEntrance();
int StadySkillMenu(int x, int y, int w);

// ---- 菜单子程 ----
int CommonMenu(int x, int y, int w, int max);
int CommonMenu(int x, int y, int w, int max, int default_);
void ShowCommonMenu(int x, int y, int w, int max, int menu);
int CommonScrollMenu(int x, int y, int w, int max, int maxshow);
void ShowCommonScrollMenu(int x, int y, int w, int max, int maxshow, int menu, int menutop);
int CommonMenu2(int x, int y, int w);
void ShowCommonMenu2(int x, int y, int w, int menu);
int SelectOneTeamMember(int x, int y, const std::string& str, int list1, int list2);
void MenuEsc();
void ShowMenu(int menu);
void MenuMedcine();
void MenuMedPoision();
bool MenuItem(int menu);
int ReadItemList(int ItemType);
void ShowMenuItem(int row, int col, int x, int y, int atlu);
void DrawItemFrame(int x, int y);
void UseItem(int inum);
bool CanEquip(int rnum, int inum);
void MenuStatus();
void ShowStatus(int rnum);
void MenuSystem();
void ShowMenuSystem(int menu);
void MenuLoad();
bool MenuLoadAtBeginning();
void MenuSave();
void MenuQuit();
void XorCount(uint8_t* Data, uint8_t xornum, int length);
bool MenuDifficult();
int TitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow);
void ShowTitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow, int menu, int menutop);

// ---- 物品效果 ----
void EffectMedcine(int role1, int role2);
void EffectMedPoision(int role1, int role2);
void EatOneItem(int rnum, int inum);

// ---- 事件 ----
void CallEvent(int num);
void ShowSaveSuccess();
void CheckHotkey(uint32 key);
void FourPets();
bool PetStatus(int r, int& menu);
void ShowPetStatus(int r, int p);
void DrawFrame(int x, int y, int w, uint32 color);
void PetLearnSkill(int r, int s);
void ResistTheater();
void ShowSkillMenu(int menu);

// ---- 云 ----
void CloudCreate(int num);
void CloudCreateOnSide(int num);

// ---- 兼容 ----
bool FileExistsUTF8(const char* filename);
bool FileExistsUTF8(const std::string& filename);