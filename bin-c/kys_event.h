#pragma once
// kys_event.h - 事件系统声明
// 对应 kys_event.pas interface

#include "kys_type.h"
#include <string>
#include <cstdint>

// 事件指令系统
void instruct_0();
void instruct_1(int talknum, int headnum, int dismode);
void instruct_2(int inum, int amount);
void ReArrangeItem();
void instruct_3(int* list, int count);
int  instruct_4(int inum, int jump1, int jump2);
int  instruct_5(int jump1, int jump2);
int  instruct_6(int battlenum, int jump1, int jump2, int getexp);
void instruct_8(int musicnum);
int  instruct_9(int jump1, int jump2);
void instruct_10(int rnum);
int  instruct_11(int jump1, int jump2);
void instruct_12();
void instruct_13();
void instruct_14();
void instruct_15();
int  instruct_16(int rnum, int jump1, int jump2);
void instruct_17(int* list, int count);
int  instruct_18(int inum, int jump1, int jump2);
void instruct_19(int x, int y);
int  instruct_20(int jump1, int jump2);
void instruct_21(int rnum);
void instruct_22();
void instruct_23(int rnum, int Poision);
void instruct_24();
void instruct_25(int x1, int y1, int x2, int y2);
void instruct_26(int snum, int enum_, int add1, int add2, int add3);
void instruct_27(int enum_, int beginpic, int endpic);
int  instruct_28(int rnum, int e1, int e2, int jump1, int jump2);
int  instruct_29(int rnum, int r1, int r2, int jump1, int jump2);
void instruct_30(int x1, int y1, int x2, int y2);
int  instruct_31(int moneynum, int jump1, int jump2);
void instruct_32(int inum, int amount);
void instruct_33(int rnum, int magicnum, int dismode);
void instruct_34(int rnum, int iq);
void instruct_35(int rnum, int magiclistnum, int magicnum, int exp);
int  instruct_36(int sexual, int jump1, int jump2);
void instruct_37(int Ethics);
void instruct_38(int snum, int layernum, int oldpic, int newpic);
void instruct_39(int snum);
void instruct_40(int director);
void instruct_41(int rnum, int inum, int amount);
int  instruct_42(int jump1, int jump2);
int  instruct_43(int inum, int jump1, int jump2);
void instruct_44(int enum1, int beginpic1, int endpic1, int enum2, int beginpic2, int endpic2);
void instruct_45(int rnum, int speed);
void instruct_46(int rnum, int mp);
void instruct_47(int rnum, int attack);
void instruct_48(int rnum, int hp);
void instruct_49(int rnum, int MPpro);
int  instruct_50(int* list, int count);
void instruct_51();
void instruct_52();
void instruct_53();
void instruct_54();
int  instruct_55(int enum_, int Value, int jump1, int jump2);
void instruct_56(int Repute);
void instruct_58();
void instruct_59();
int  instruct_60(int snum, int enum_, int pic, int jump1, int jump2);
void instruct_62();
void EndAmi();
void instruct_63(int rnum, int sexual);
void instruct_64();
void instruct_66(int musicnum);
void instruct_67(int Soundnum);

// 扩展指令
int  e_GetValue(int bit, int t, int x);
int  instruct_50e(int code, int e1, int e2, int e3, int e4, int e5, int e6);

// 武功/功体系统
void StudyMagic(int rnum, int magicnum, int newmagicnum, int level, int dismode);
bool HaveMagic(int person, int mnum, int lv);
int  GetMagicLevel(int person, int mnum);
bool GetGongtiState(int person, int state);
int  GetGongtiLevel(int person, int mnum);
void SetGongti(int rnum, int mnum);
void StudyGongti();
void ShowStudyGongti(int menu, int menu2, int max);
int  StadyGongtiMenu(int x, int y, int w);
void GongtiLevelUp(int rnum, int mnum);
bool GetEquipState(int person, int state);
void AddSkillPoint(int num);
bool AddBattleStateToEquip();

// 对话/名称/文本
void NewTalk0(int headnum, int talknum, int namenum, int place, int showhead, int color, int frame);
void NewTalk(int headnum, int talknum, int namenum, int place, int showhead, int color, int frame,
    const std::wstring& content = L"", const std::wstring& disname = L"");
int  ReSetName(int t, int inum, int newnamenum);
void ShowTitle(int talknum, int color);
std::wstring ReadTalk(int talknum);
void DivideName(const std::wstring& fullname, std::wstring& surname, std::wstring& givenname);
std::wstring ReplaceStr(const std::wstring& S, const std::wstring& Srch, const std::wstring& Replace);

// 场景辅助
void JmpScene(int snum, int y, int x);
void SetScene();
void chengesnowhill();

// 谜题/游戏
void Puzzle();
int  SelectList(int begintalknum, int amount);

// 属性查询
int  GetItemCount(int inum);
bool GetPetSkill(int rnum, int skill);
int  GetRoleSpeed(int rnum, bool Equip);
int  GetRoleDefence(int rnum, bool Equip);
int  GetRoleAttack(int rnum, bool Equip);
int  GetRoleHidWeapon(int rnum, bool Equip);
int  GetRoleUnusual(int rnum, bool Equip);
int  GetRoleKnife(int rnum, bool Equip);
int  GetRoleSword(int rnum, bool Equip);
int  GetRoleFist(int rnum, bool Equip);
int  GetRoleDefPoi(int rnum, bool Equip);
int  GetRoleUsePoi(int rnum, bool Equip);
int  GetRoleMedPoi(int rnum, bool Equip);
int  GetRoleMedcine(int rnum, bool Equip);
int  GetRoleAttPoi(int rnum, bool Equip);
int  CheckEquipSet(int e0, int e1, int e2, int e3);

// 输入
int  InputAmount();
bool EnterString(std::string& str, int x, int y, int w, int h);