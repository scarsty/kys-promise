// kys_event.cpp - 事件系统实现
// 对应 kys_event.pas implementation

#include "kys_event.h"
#include "kys_engine.h"
#include "kys_battle.h"
#include "kys_main.h"
#include "kys_littlegame.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// 文件辅助
static FILE* PasOpen(const std::string& path, const char* mode)
{
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), mode);
    return f;
}

// ==== instruct_0 - 重绘 ====
void instruct_0()
{
    Redraw();
}

// ==== instruct_1 - 对话 ====
void instruct_1(int talknum, int headnum, int dismode)
{
    int headx = 40, heady = 80, diagx = 100, diagy = 30;
    switch (dismode)
    {
    case 0: headx = 40; heady = 80; diagx = 100; diagy = 30; break;
    case 1: headx = 546; heady = CENTER_Y * 2 - 80; diagx = 10; diagy = CENTER_Y * 2 - 130; break;
    case 2: headx = -1; heady = -1; diagx = 100; diagy = 30; break;
    case 3: headx = -1; heady = -1; diagx = 100; diagy = CENTER_Y * 2 - 130; break;
    case 4: headx = 546; heady = 80; diagx = 10; diagy = 30; break;
    case 5: headx = 40; heady = CENTER_Y * 2 - 80; diagx = 100; diagy = CENTER_Y * 2 - 130; break;
    }

    FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
    FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return; }

    int offset = 0, len = 0;
    if (talknum == 0) { fread(&len, 4, 1, idx); offset = 0; }
    else
    {
        fseek(idx, (talknum - 1) * 4, SEEK_SET);
        fread(&offset, 4, 1, idx);
        fread(&len, 4, 1, idx);
    }
    len -= offset;
    std::vector<uint8_t> talkarray(len + 1);
    fseek(grp, offset, SEEK_SET);
    fread(talkarray.data(), 1, len, grp);
    fclose(idx); fclose(grp);

    DrawRectangleWithoutFrame(0, diagy - 10, 640, 120, 0, 40);
    if (headx > 0) DrawHeadPic(headnum, headx, heady);

    for (int i = 0; i < len; i++)
    {
        talkarray[i] ^= 0xFF;
        if (talkarray[i] == 0x2A) talkarray[i] = 0;
    }

    int p = 0, l = 0;
    for (int i = 0; i <= len; i++)
    {
        if (talkarray[i] == 0)
        {
            DrawGBKShadowText(reinterpret_cast<const char*>(&talkarray[p]),
                diagx, diagy + l * 22, ColColor(0xFF), ColColor(0x0));
            p = i + 1;
            l++;
            if (l >= 4 && i < len)
            {
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                WaitAnyKey();
                Redraw();
                DrawRectangleWithoutFrame(0, diagy - 10, 640, 120, 0, 40);
                if (headx > 0) DrawHeadPic(headnum, headx, heady);
                l = 0;
            }
        }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    Redraw();
}

// ==== instruct_2 - 得到/失去物品 ====
void instruct_2(int inum, int amount)
{
    int i = 0;
    while (RItemList[i].Number >= 0 && i < MAX_ITEM_AMOUNT)
    {
        if (RItemList[i].Number == inum)
        {
            RItemList[i].Amount += amount;
            if (RItemList[i].Amount < 0 && amount >= 0) RItemList[i].Amount = 32767;
            if (RItemList[i].Amount < 0 && amount < 0) RItemList[i].Amount = 0;
            break;
        }
        i++;
    }
    if (RItemList[i].Number < 0)
    {
        RItemList[i].Number = inum;
        RItemList[i].Amount = amount;
    }
    ReArrangeItem();

    int x = CENTER_X - 25;
    DrawRectangle(x - 75, 98, 245, 76, 0, ColColor(0xFF), 30);
    std::wstring word;
    int dispAmount = amount;
    if (amount >= 0) word = L" 得到物品";
    else { word = L" 失去物品"; dispAmount = -amount; }
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), x - 90, 100, ColColor(0x21), ColColor(0x23));
    DrawGBKShadowText(reinterpret_cast<const char*>(RItem[inum].Name), x - 90, 125, ColColor(0x5), ColColor(0x7));
    word = L" 數量";
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), x - 90, 150, ColColor(0x64), ColColor(0x66));
    word = std::format(L" {:5d}", dispAmount);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(word.data()), x - 5, 150, ColColor(0x64), ColColor(0x66));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
}

// ==== ReArrangeItem ====
void ReArrangeItem()
{
    std::vector<int> item(MAX_ITEM_AMOUNT), amount(MAX_ITEM_AMOUNT);
    int p = 0;
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number >= 0 && RItemList[i].Amount > 0)
        {
            item[p] = RItemList[i].Number;
            amount[p] = RItemList[i].Amount;
            p++;
        }
    }
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (i < p) { RItemList[i].Number = item[i]; RItemList[i].Amount = amount[i]; }
        else { RItemList[i].Number = -1; RItemList[i].Amount = 0; }
    }
}

// ==== instruct_3 - 修改事件数据 ====
void instruct_3(int* list, int count)
{
    if (list[0] == -2) list[0] = CurScene;
    if (list[1] == -2) list[1] = CurEvent;
    if (count > 11 && list[11] == -2) list[11] = DData[list[0]][list[1]][9];
    if (count > 12 && list[12] == -2) list[12] = DData[list[0]][list[1]][10];

    if (DData[list[0]][list[1]][10] > 0 && DData[list[0]][list[1]][9] >= 0)
        SData[list[0]][3][DData[list[0]][list[1]][10]][DData[list[0]][list[1]][9]] = -1;

    int oldpic = DData[list[0]][list[1]][5];
    int newpic = list[7];

    if (list[0] == CurScene &&
        (DData[list[0]][list[1]][9] != list[11] || DData[list[0]][list[1]][10] != list[12]))
        UpdateScene(DData[list[0]][list[1]][10], DData[list[0]][list[1]][9], oldpic, 0);

    for (int i = 0; i <= 10; i++)
    {
        if (list[2 + i] != -2)
            DData[list[0]][list[1]][i] = list[2 + i];
    }

    if (DData[list[0]][list[1]][10] > 0 && DData[list[0]][list[1]][9] >= 0)
        SData[list[0]][3][DData[list[0]][list[1]][10]][DData[list[0]][list[1]][9]] = list[1];

    if (!((Sx == RScene[CurScene].ExitX[0] && Sy == RScene[CurScene].ExitY[0]) ||
          (Sx == RScene[CurScene].ExitX[1] && Sy == RScene[CurScene].ExitY[1]) ||
          (Sx == RScene[CurScene].ExitX[2] && Sy == RScene[CurScene].ExitY[2])))
    {
        if (list[0] == CurScene && (list[8] != -2 || list[9] != -2 || list[7] != -2 || list[11] != -2 || list[10] != -2))
            UpdateScene(list[12], list[11], oldpic, newpic);
    }
}

// ==== instruct_4 - 是否使用剧情物品 ====
int instruct_4(int inum, int jump1, int jump2)
{
    return (inum == CurItem) ? jump1 : jump2;
}

// ==== instruct_5 - 询问战斗 ====
int instruct_5(int jump1, int jump2)
{
    MenuString.resize(3);
    MenuString[1] = L" 戰鬥";
    MenuString[0] = L" 取消";
    MenuString[2] = L" 是否與之戰鬥？";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(MenuString[2].data()),
        CENTER_X - 75, CENTER_Y - 85, 150, ColColor(0x5), ColColor(0x7));
    int menu = CommonMenu2(CENTER_X - 49, CENTER_Y - 50, 98);
    int result = (menu == 1) ? jump1 : jump2;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    return result;
}

// ==== instruct_6 - 战斗 ====
int instruct_6(int battlenum, int jump1, int jump2, int getexp)
{
    return Battle(battlenum, getexp) ? jump1 : jump2;
}

// ==== instruct_8 - 设置退出场景音乐 ====
void instruct_8(int musicnum)
{
    ExitSceneMusicNum = musicnum;
}

// ==== instruct_9 - 询问加入 ====
int instruct_9(int jump1, int jump2)
{
    MenuString.resize(3);
    MenuString[1] = L" 要求";
    MenuString[0] = L" 取消";
    MenuString[2] = L" 是否要求加入？";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(MenuString[2].data()),
        CENTER_X - 75, CENTER_Y - 85, 150, ColColor(0x5), ColColor(0x7));
    int menu = CommonMenu2(CENTER_X - 49, CENTER_Y - 50, 98);
    int result = (menu == 1) ? jump1 : jump2;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    return result;
}

// ==== instruct_10 - 加入队友 ====
void instruct_10(int rnum)
{
    RRole[rnum].TeamState = 2;
    for (int i1 = 0; i1 < 4; i1++)
    {
        if (RRole[rnum].TakingItem[i1] >= 0 && RRole[rnum].TakingItemAmount[i1] > 0)
        {
            instruct_2(RRole[rnum].TakingItem[i1], RRole[rnum].TakingItemAmount[i1]);
            Redraw();
            RRole[rnum].TakingItem[i1] = -1;
            RRole[rnum].TakingItemAmount[i1] = 0;
        }
    }
    for (int i = 0; i < 6; i++)
    {
        if (TeamList[i] == rnum) { RRole[rnum].TeamState = 1; break; }
        if (TeamList[i] < 0) { TeamList[i] = rnum; RRole[rnum].TeamState = 1; break; }
    }
}

// ==== instruct_11 - 询问住宿 ====
int instruct_11(int jump1, int jump2)
{
    MenuString.resize(3);
    MenuString[1] = L" 要求";
    MenuString[0] = L" 取消";
    MenuString[2] = L" 是否需要住宿？";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(MenuString[2].data()),
        CENTER_X - 75, CENTER_Y - 85, 150, ColColor(0x5), ColColor(0x7));
    int menu = CommonMenu2(CENTER_X - 49, CENTER_Y - 50, 98);
    int result = (menu == 1) ? jump1 : jump2;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    return result;
}

// ==== instruct_12 - 住宿恢复 ====
void instruct_12()
{
    for (int i = 0; i < 6; i++)
    {
        int rnum = TeamList[i];
        if (rnum >= 0 && !(RRole[rnum].Hurt > 33 || RRole[rnum].Poision > 33))
        {
            RRole[rnum].Hurt = 0;
            RRole[rnum].Poision = 0;
            RRole[rnum].CurrentHP = RRole[rnum].MaxHP;
            RRole[rnum].CurrentMP = RRole[rnum].MaxMP;
            RRole[rnum].PhyPower = MAX_PHYSICAL_POWER;
        }
    }
    for (int i = 0; i < static_cast<int>(RRole.size()); i++)
    {
        if (RRole[i].TeamState == 2 && !(RRole[i].Hurt > 33 || RRole[i].Poision > 33))
        {
            RRole[i].Hurt = 0;
            RRole[i].Poision = 0;
            RRole[i].CurrentHP = RRole[i].MaxHP;
            RRole[i].CurrentMP = RRole[i].MaxMP;
            RRole[i].PhyPower = MAX_PHYSICAL_POWER;
        }
    }
}

// ==== instruct_13 - 亮屏 ====
void instruct_13()
{
    InitialScene();
    DrawScene();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== instruct_14 - 黑屏 ====
void instruct_14()
{
    for (int i = 1; i <= 15; i++)
    {
        DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, static_cast<int>(i * 6.5));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(30);
    }
}

// ==== instruct_15 - 失败画面 ====
void instruct_15()
{
    DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 100);
    std::wstring word = L"三十功名塵與土";
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 80, CENTER_Y - 10, ColColor(0xFF), ColColor(0x0));
    word = L"八千里路雲和月";
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 80, CENTER_Y + 20, ColColor(0xFF), ColColor(0x0));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    SDL_Delay(3000);
}

// ==== instruct_16 - 判断是否在队 ====
int instruct_16(int rnum, int jump1, int jump2)
{
    return (RRole[rnum].TeamState == 1 || RRole[rnum].TeamState == 2) ? jump1 : jump2;
}

// ==== instruct_17 - 写场景数据 ====
void instruct_17(int* list, int count)
{
    // TODO: 完整实现
}

// ==== instruct_18 - 判断是否拥有物品 ====
int instruct_18(int inum, int jump1, int jump2)
{
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number == inum && RItemList[i].Amount > 0)
            return jump1;
    }
    return jump2;
}

// ==== instruct_19 - 设置坐标 ====
void instruct_19(int x, int y)
{
    Sx = x; Sy = y;
}

// ==== instruct_20 - 判断队伍满 ====
int instruct_20(int jump1, int jump2)
{
    for (int i = 0; i < 6; i++)
        if (TeamList[i] < 0) return jump2;
    return jump1;
}

// ==== instruct_21 - 移除队友 ====
void instruct_21(int rnum)
{
    for (int i = 0; i < 6; i++)
    {
        if (TeamList[i] == rnum)
        {
            // 归还装备
            for (int e = 0; e < 4; e++)
            {
                if (RRole[rnum].Equip[e] >= 0)
                {
                    instruct_2(RRole[rnum].Equip[e], 1);
                    Redraw();
                    RRole[rnum].Equip[e] = -1;
                }
            }
            // 归还修炼书
            if (RRole[rnum].PracticeBook >= 0)
            {
                instruct_2(RRole[rnum].PracticeBook, 1);
                Redraw();
                RRole[rnum].PracticeBook = -1;
            }
            RRole[rnum].TeamState = 0;
            TeamList[i] = -1;
            // 压缩队列
            for (int j = i; j < 5; j++)
                TeamList[j] = TeamList[j + 1];
            TeamList[5] = -1;
            break;
        }
    }
}

// ==== instruct_22 - 全队MP清零 ====
void instruct_22()
{
    for (int i = 0; i < 6; i++)
    {
        int rnum = TeamList[i];
        if (rnum >= 0) RRole[rnum].CurrentMP = 0;
    }
}

// ==== instruct_23 - 设置用毒 ====
void instruct_23(int rnum, int Poision)
{
    RRole[rnum].UsePoi = static_cast<int16_t>(Poision);
}

void instruct_24() { /* 空实现 */ }

// ==== instruct_25 - 场景滚动动画 ====
void instruct_25(int x1, int y1, int x2, int y2)
{
    // TODO: 完整滚动实现
}

// ==== instruct_26 - 修改事件D数据 ====
void instruct_26(int snum, int enum_, int add1, int add2, int add3)
{
    DData[snum][enum_][0] += add1;
    DData[snum][enum_][1] += add2;
    DData[snum][enum_][2] += add3;
}

// ==== instruct_27 - 事件动画 ====
void instruct_27(int enum_, int beginpic, int endpic)
{
    // TODO
}

// ==== instruct_28 - 判断道德范围 ====
int instruct_28(int rnum, int e1, int e2, int jump1, int jump2)
{
    return (RRole[rnum].Ethics >= e1 && RRole[rnum].Ethics <= e2) ? jump1 : jump2;
}

// ==== instruct_29 - 判断攻击力范围 ====
int instruct_29(int rnum, int r1, int r2, int jump1, int jump2)
{
    return (RRole[rnum].Attack >= r1 && RRole[rnum].Attack <= r2) ? jump1 : jump2;
}

// ==== instruct_30 - 自动行走 ====
void instruct_30(int x1, int y1, int x2, int y2)
{
    // TODO: 完整寻路实现
}

// ==== instruct_31 - 判断金钱 ====
int instruct_31(int moneynum, int jump1, int jump2)
{
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number == MONEY_ID && RItemList[i].Amount >= moneynum)
            return jump1;
    }
    return jump2;
}

// ==== instruct_32 - 修改物品数量 ====
void instruct_32(int inum, int amount)
{
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number == inum)
        {
            RItemList[i].Amount += amount;
            if (RItemList[i].Amount < 0) RItemList[i].Amount = 0;
            break;
        }
    }
    ReArrangeItem();
}

// ==== instruct_33 - 学习武功 ====
void instruct_33(int rnum, int magicnum, int dismode)
{
    StudyMagic(rnum, magicnum, magicnum, 0, dismode);
}

// ==== instruct_34 - 增加资质 ====
void instruct_34(int rnum, int iq)
{
    RRole[rnum].Aptitude += static_cast<int16_t>(iq);
    if (iq > 0)
    {
        std::wstring word = std::format(L" 資質增加{}", iq);
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey();
        Redraw();
    }
}

// ==== instruct_35 - 设置武功和经验 ====
void instruct_35(int rnum, int magiclistnum, int magicnum, int exp)
{
    if (magiclistnum >= 0 && magiclistnum < 10)
    {
        RRole[rnum].Magic[magiclistnum] = static_cast<int16_t>(magicnum);
        RRole[rnum].MagLevel[magiclistnum] = static_cast<int16_t>(exp);
    }
}

// ==== instruct_36 - 判断性别 ====
int instruct_36(int sexual, int jump1, int jump2)
{
    return (RRole[0].Sexual == sexual) ? jump1 : jump2;
}

// ==== instruct_37 - 修改道德 ====
void instruct_37(int Ethics)
{
    RRole[0].Ethics += static_cast<int16_t>(Ethics);
}

// ==== instruct_38 - 替换场景贴图 ====
void instruct_38(int snum, int layernum, int oldpic, int newpic)
{
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            if (SData[snum][layernum][x][y] == oldpic)
                SData[snum][layernum][x][y] = static_cast<int16_t>(newpic);
}

// ==== instruct_39 - 清除场景进入条件 ====
void instruct_39(int snum)
{
    RScene[snum].MainEntranceX1 = 0;
    RScene[snum].MainEntranceY1 = 0;
    RScene[snum].MainEntranceX2 = 0;
    RScene[snum].MainEntranceY2 = 0;
}

// ==== instruct_40 - 设置朝向 ====
void instruct_40(int director) { SFace = director; }

// ==== instruct_41 - 修改角色携带物品 ====
void instruct_41(int rnum, int inum, int amount)
{
    for (int i = 0; i < 4; i++)
    {
        if (RRole[rnum].TakingItem[i] == inum || RRole[rnum].TakingItem[i] < 0)
        {
            RRole[rnum].TakingItem[i] = static_cast<int16_t>(inum);
            RRole[rnum].TakingItemAmount[i] = static_cast<int16_t>(amount);
            break;
        }
    }
}

// ==== instruct_42 - 判断队中是否有女性 ====
int instruct_42(int jump1, int jump2)
{
    for (int i = 0; i < 6; i++)
        if (TeamList[i] >= 0 && RRole[TeamList[i]].Sexual == 1) return jump1;
    return jump2;
}

// ==== instruct_43 - 判断是否拥有物品 ====
int instruct_43(int inum, int jump1, int jump2)
{
    return instruct_18(inum, jump1, jump2);
}

// ==== instruct_44 - 双事件同步动画 ====
void instruct_44(int enum1, int beginpic1, int endpic1, int enum2, int beginpic2, int endpic2)
{
    // TODO
}

// ==== instruct_45~49 - 增加属性 ====
void instruct_45(int rnum, int speed)
{
    RRole[rnum].Speed += static_cast<int16_t>(speed);
    if (speed > 0)
    {
        std::wstring word = std::format(L" 輕功增加{}", speed);
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey(); Redraw();
    }
}

void instruct_46(int rnum, int mp)
{
    RRole[rnum].MaxMP += static_cast<int16_t>(mp);
    RRole[rnum].CurrentMP = RRole[rnum].MaxMP;
    if (mp > 0)
    {
        std::wstring word = std::format(L" 內力增加{}", mp);
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey(); Redraw();
    }
}

void instruct_47(int rnum, int attack)
{
    RRole[rnum].Attack += static_cast<int16_t>(attack);
    if (attack > 0)
    {
        std::wstring word = std::format(L" 武力增加{}", attack);
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey(); Redraw();
    }
}

void instruct_48(int rnum, int hp)
{
    RRole[rnum].MaxHP += static_cast<int16_t>(hp);
    RRole[rnum].CurrentHP = RRole[rnum].MaxHP;
    if (hp > 0)
    {
        std::wstring word = std::format(L" 生命增加{}", hp);
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey(); Redraw();
    }
}

void instruct_49(int rnum, int MPpro)
{
    RRole[rnum].MPType = static_cast<int16_t>(MPpro);
}

// ==== instruct_50 - 扩展指令分发器 ====
int instruct_50(int* list, int count)
{
    if (count >= 7 && list[0] <= 128)
        return instruct_50e(list[0], list[1], list[2], list[3], list[4], list[5], list[6]);
    // 判断5物品齐全逻辑
    // TODO
    return 0;
}

void instruct_51() { /* 随机语录 TODO */ }
void instruct_52() { /* 品德指数 TODO */ }
void instruct_53() { /* 声望指数 TODO */ }
void instruct_54() { /* 开放全部场景 TODO */ }

int instruct_55(int enum_, int Value, int jump1, int jump2)
{
    return (DData[CurScene][enum_][0] == Value) ? jump1 : jump2;
}

void instruct_56(int Repute) { /* 增加声望 TODO */ }
void instruct_58() { /* 华山论剑 TODO */ }

void instruct_59()
{
    for (int i = 1; i < 6; i++)
    {
        if (TeamList[i] >= 0)
        {
            instruct_21(TeamList[i]);
            i--; // 重新检查此位置
        }
    }
}

int instruct_60(int snum, int enum_, int pic, int jump1, int jump2)
{
    return (DData[snum][enum_][5] == pic) ? jump1 : jump2;
}

void instruct_62() { /* 结局动画 TODO */ }
void EndAmi() { /* 结局制作人名单 TODO */ }
void instruct_63(int rnum, int sexual) { RRole[rnum].Sexual = static_cast<int16_t>(sexual); }
void instruct_64() { /* 韦小宝商店 TODO */ }
void instruct_66(int musicnum) { PlayMP3(musicnum, -1); }
void instruct_67(int Soundnum) { PlaySoundEffect(Soundnum); }

// ==== e_GetValue - 参数解析器 ====
int e_GetValue(int bit, int t, int x)
{
    int i = t & (1 << bit);
    return (i == 0) ? x : x50[x];
}

// ==== instruct_50e - 扩展指令 (巨型switch) ====
int instruct_50e(int code, int e1, int e2, int e3, int e4, int e5, int e6)
{
    int result = 0;
    int t1;
    switch (code)
    {
    case 0: // 赋值
        x50[e1] = e2;
        break;
    case 1: // 写参数组成员
        t1 = e3 + e_GetValue(0, e1, e4);
        x50[t1] = e_GetValue(1, e1, e5);
        if (e2 == 1) x50[t1] &= 0xFF;
        break;
    case 2: // 读参数组成员
        t1 = e3 + e_GetValue(0, e1, e4);
        x50[e5] = x50[t1];
        if (e2 == 1) x50[t1] &= 0xFF;
        break;
    case 3: // 基本运算
        t1 = e_GetValue(0, e1, e5);
        switch (e2)
        {
        case 0: x50[e3] = x50[e4] + t1; break;
        case 1: x50[e3] = x50[e4] - t1; break;
        case 2: x50[e3] = x50[e4] * t1; break;
        case 3: x50[e3] = (t1 != 0) ? x50[e4] / t1 : 0; break;
        case 4: x50[e3] = (t1 != 0) ? x50[e4] % t1 : 0; break;
        case 5: x50[e3] = (t1 != 0) ? static_cast<int>(static_cast<uint16_t>(x50[e4]) / t1) : 0; break;
        }
        break;
    case 4: // 判断
        x50[0x7000] = 0;
        t1 = e_GetValue(0, e1, e4);
        switch (e2)
        {
        case 0: if (!(x50[e3] < t1)) x50[0x7000] = 1; break;
        case 1: if (!(x50[e3] <= t1)) x50[0x7000] = 1; break;
        case 2: if (!(x50[e3] == t1)) x50[0x7000] = 1; break;
        case 3: if (!(x50[e3] != t1)) x50[0x7000] = 1; break;
        case 4: if (!(x50[e3] >= t1)) x50[0x7000] = 1; break;
        case 5: if (!(x50[e3] > t1)) x50[0x7000] = 1; break;
        case 6: x50[0x7000] = 0; break;
        case 7: x50[0x7000] = 1; break;
        }
        break;
    case 5: // 清零全部参数
        std::memset(x50, 0, sizeof(x50));
        break;
    case 8: // 读对话到字符串
    {
        t1 = e_GetValue(0, e1, e2);
        FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
        FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
        if (idx && grp)
        {
            int offset = 0, len = 0;
            if (t1 == 0) { fread(&len, 4, 1, idx); }
            else { fseek(idx, (t1 - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
            len -= offset;
            std::vector<uint8_t> buf(len + 2);
            fseek(grp, offset, SEEK_SET);
            fread(buf.data(), 1, len, grp);
            for (int i = 0; i < len; i++) buf[i] ^= 0xFF;
            buf[len] = 0;
            // 存入 x50 字符串区
            int16_t* dst = &x50[e3];
            std::memcpy(dst, buf.data(), std::min(len + 1, 256));
        }
        if (idx) fclose(idx);
        if (grp) fclose(grp);
        break;
    }
    case 16: // 写R数据
    {
        int16_t* base = nullptr; int sz = 0;
        switch (e2)
        {
        case 0: base = reinterpret_cast<int16_t*>(RRole.data()); sz = sizeof(TRole) / 2; break;
        case 1: base = reinterpret_cast<int16_t*>(RItem.data()); sz = sizeof(TItem) / 2; break;
        case 2: base = reinterpret_cast<int16_t*>(RScene.data()); sz = sizeof(TScene) / 2; break;
        case 3: base = reinterpret_cast<int16_t*>(RMagic.data()); sz = sizeof(TMagic) / 2; break;
        case 4: base = reinterpret_cast<int16_t*>(RShop.data()); sz = sizeof(TShop) / 2; break;
        }
        if (base)
        {
            t1 = e_GetValue(0, e1, e3);
            int idx_ = e_GetValue(1, e1, e4);
            base[t1 * sz + idx_] = static_cast<int16_t>(e_GetValue(2, e1, e5));
        }
        break;
    }
    case 17: // 读R数据
    {
        int16_t* base = nullptr; int sz = 0;
        switch (e2)
        {
        case 0: base = reinterpret_cast<int16_t*>(RRole.data()); sz = sizeof(TRole) / 2; break;
        case 1: base = reinterpret_cast<int16_t*>(RItem.data()); sz = sizeof(TItem) / 2; break;
        case 2: base = reinterpret_cast<int16_t*>(RScene.data()); sz = sizeof(TScene) / 2; break;
        case 3: base = reinterpret_cast<int16_t*>(RMagic.data()); sz = sizeof(TMagic) / 2; break;
        case 4: base = reinterpret_cast<int16_t*>(RShop.data()); sz = sizeof(TShop) / 2; break;
        }
        if (base)
        {
            t1 = e_GetValue(0, e1, e3);
            int idx_ = e_GetValue(1, e1, e4);
            x50[e5] = base[t1 * sz + idx_];
        }
        break;
    }
    case 18: // 写队伍数据
        t1 = e_GetValue(0, e1, e3);
        TeamList[t1] = static_cast<int16_t>(e_GetValue(1, e1, e4));
        break;
    case 19: // 读队伍数据
        t1 = e_GetValue(0, e1, e3);
        x50[e4] = TeamList[t1];
        break;
    case 20: // 获取物品数量
        x50[e3] = GetItemCount(e_GetValue(0, e1, e2));
        break;
    case 21: // 写场景事件Ddata
    {
        int s = e_GetValue(0, e1, e2);
        int ev = e_GetValue(1, e1, e3);
        int idx_ = e_GetValue(2, e1, e4);
        DData[s][ev][idx_] = static_cast<int16_t>(e_GetValue(3, e1, e5));
        break;
    }
    case 22: // 读场景事件Ddata
    {
        int s = e_GetValue(0, e1, e2);
        int ev = e_GetValue(1, e1, e3);
        int idx_ = e_GetValue(2, e1, e4);
        x50[e5] = DData[s][ev][idx_];
        break;
    }
    case 23: // 写场景Sdata
    {
        int s = e_GetValue(0, e1, e2);
        int layer = e_GetValue(1, e1, e3);
        int sx_ = e_GetValue(2, e1, e4);
        int sy_ = e_GetValue(3, e1, e5);
        SData[s][layer][sx_][sy_] = static_cast<int16_t>(e_GetValue(4, e1, e6));
        break;
    }
    case 24: // 读场景Sdata
    {
        int s = e_GetValue(0, e1, e2);
        int layer = e_GetValue(1, e1, e3);
        int sx_ = e_GetValue(2, e1, e4);
        int sy_ = e_GetValue(3, e1, e5);
        x50[e6] = SData[s][layer][sx_][sy_];
        break;
    }
    case 26: // 读全局变量
    {
        switch (e2)
        {
        case 0: x50[e3] = CurScene; break;
        case 1: x50[e3] = Sx; break;
        case 2: x50[e3] = Sy; break;
        case 3: x50[e3] = Mx; break;
        case 4: x50[e3] = My; break;
        case 5: x50[e3] = SFace; break;
        case 6: x50[e3] = Ax; break;
        case 7: x50[e3] = Ay; break;
        case 8: x50[e3] = BRoleAmount; break;
        case 9: x50[e3] = CurEvent; break;
        case 10: x50[e3] = gametime; break;
        }
        break;
    }
    case 33: // 绘制GBK文本
    {
        // TODO: 完整实现
        break;
    }
    case 34: // 绘制矩形
    {
        int rx = e_GetValue(0, e1, e2);
        int ry = e_GetValue(1, e1, e3);
        int rw = e_GetValue(2, e1, e4);
        int rh = e_GetValue(3, e1, e5);
        DrawRectangleWithoutFrame(rx, ry, rw, rh, 0, 60);
        break;
    }
    case 35: // 等待按键
    {
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        x50[e2] = WaitAnyKey();
        break;
    }
    case 37: // 延迟
        SDL_Delay(e_GetValue(0, e1, e2));
        break;
    case 38: // 随机数
        x50[e3] = rand() % (e_GetValue(0, e1, e2) + 1);
        break;
    case 42: // 设置大地图坐标
        Mx = e_GetValue(0, e1, e2);
        My = e_GetValue(1, e1, e3);
        break;
    case 43: // 调用事件/小游戏
    {
        t1 = e_GetValue(0, e1, e2);
        if (t1 >= 0)
        {
            CallEvent(t1);
        }
        else
        {
            // 特殊子指令
            switch (t1)
            {
            case -1: JmpScene(e_GetValue(1, e1, e3), e_GetValue(2, e1, e4), e_GetValue(3, e1, e5)); break;
            case -2: x50[0x7000] = Poetry(e_GetValue(1, e1, e3), e_GetValue(2, e1, e4), e_GetValue(3, e1, e5), e_GetValue(4, e1, e6)) ? 0 : 1; break;
            case -4: x50[0x7000] = Acupuncture(e_GetValue(1, e1, e3)) ? 0 : 1; break;
            case -9: x50[0x7000] = ShotEagle(e_GetValue(1, e1, e3), e_GetValue(2, e1, e4)) ? 0 : 1; break;
            case -10: x50[0x7000] = rotoSpellPicture(e_GetValue(1, e1, e3), e_GetValue(2, e1, e4)) ? 0 : 1; break;
            case -21: x50[e3] = FemaleSnake(); break;
            default:
                // TODO: 其他负值子指令
                break;
            }
        }
        break;
    }
    default:
        // TODO: 其他 code
        break;
    }
    return result;
}

// ==== HaveMagic ====
bool HaveMagic(int person, int mnum, int lv)
{
    for (int i = 0; i < 10; i++)
    {
        if (RRole[person].Magic[i] == mnum && RRole[person].MagLevel[i] >= lv)
            return true;
    }
    return false;
}

// ==== GetMagicLevel ====
int GetMagicLevel(int person, int mnum)
{
    for (int i = 0; i < 10; i++)
    {
        if (RRole[person].Magic[i] == mnum)
            return RRole[person].MagLevel[i];
    }
    return 0;
}

// ==== GetGongtiLevel ====
int GetGongtiLevel(int person, int mnum)
{
    int ml = GetMagicLevel(person, mnum);
    return std::min(static_cast<int>(RMagic[mnum].MaxLevel), ml / 100);
}

// ==== SetGongti ====
void SetGongti(int rnum, int mnum)
{
    // TODO: 完整实现
}

// ==== GetGongtiState ====
bool GetGongtiState(int person, int state)
{
    // TODO: 完整实现
    return false;
}

// ==== GetEquipState ====
bool GetEquipState(int person, int state)
{
    for (int i = 0; i < 4; i++)
    {
        int e = RRole[person].Equip[i];
        if (e >= 0 && RItem[e].BattleEffect == state)
            return true;
    }
    return false;
}

// ==== StudyMagic ====
void StudyMagic(int rnum, int magicnum, int newmagicnum, int level, int dismode)
{
    if (newmagicnum == 0)
    {
        // 删除武功
        for (int i = 0; i < 10; i++)
        {
            if (RRole[rnum].Magic[i] == magicnum)
            {
                for (int j = i; j < 9; j++)
                {
                    RRole[rnum].Magic[j] = RRole[rnum].Magic[j + 1];
                    RRole[rnum].MagLevel[j] = RRole[rnum].MagLevel[j + 1];
                }
                RRole[rnum].Magic[9] = 0;
                RRole[rnum].MagLevel[9] = 0;
                break;
            }
        }
        return;
    }

    // 查找是否已有
    for (int i = 0; i < 10; i++)
    {
        if (RRole[rnum].Magic[i] == newmagicnum)
        {
            RRole[rnum].MagLevel[i] += static_cast<int16_t>(level);
            if (RRole[rnum].MagLevel[i] > 999) RRole[rnum].MagLevel[i] = 999;
            if (dismode == 0)
            {
                // 显示升级提示
            }
            return;
        }
    }

    // 替换旧武功或添加到空位
    for (int i = 0; i < 10; i++)
    {
        if (RRole[rnum].Magic[i] == magicnum || RRole[rnum].Magic[i] <= 0)
        {
            RRole[rnum].Magic[i] = static_cast<int16_t>(newmagicnum);
            RRole[rnum].MagLevel[i] = static_cast<int16_t>(level);
            break;
        }
    }

    if (dismode == 0)
    {
        std::wstring word = L" 學會武功";
        DrawTextWithRect(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 75, CENTER_Y - 30, 150,
            ColColor(0x5), ColColor(0x7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey();
        Redraw();
    }
}

// ==== GetItemCount ====
int GetItemCount(int inum)
{
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number == inum)
            return RItemList[i].Amount;
    }
    return 0;
}

// ==== GetPetSkill ====
bool GetPetSkill(int rnum, int skill)
{
    // TODO: 完整实现
    return false;
}

// ==== ReadTalk ====
std::wstring ReadTalk(int talknum)
{
    FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
    FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return L""; }
    int offset = 0, len = 0;
    if (talknum == 0) { fread(&len, 4, 1, idx); }
    else { fseek(idx, (talknum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
    len -= offset;
    std::vector<uint8_t> buf(len + 2);
    fseek(grp, offset, SEEK_SET);
    fread(buf.data(), 1, len, grp);
    fclose(idx); fclose(grp);
    for (int i = 0; i < len; i++)
    {
        buf[i] ^= 0xFF;
        if (buf[i] == 0xFF) buf[i] = 0;
    }
    buf[len] = 0;
    return GBKToUnicode(reinterpret_cast<const char*>(buf.data()));
}

// ==== JmpScene ====
void JmpScene(int snum, int y, int x)
{
    CurScene = snum;
    Sx = x; Sy = y;
    resetpallet();
    InitialScene();
    DrawScene();
    ShowSceneName(snum);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== NewTalk0 ====
void NewTalk0(int headnum, int talknum, int namenum, int place, int showhead, int color, int frame)
{
    // TODO: 完整实现
}

// ==== NewTalk ====
void NewTalk(int headnum, int talknum, int namenum, int place, int showhead, int color, int frame,
    const std::wstring& content, const std::wstring& disname)
{
    // TODO: 完整实现
}

// ==== ShowTitle ====
void ShowTitle(int talknum, int color)
{
    // TODO: 完整实现
}

// ==== ReSetName ====
int ReSetName(int t, int inum, int newnamenum)
{
    // TODO: 完整实现
    return 0;
}

// ==== DivideName / ReplaceStr ====
void DivideName(const std::wstring& fullname, std::wstring& surname, std::wstring& givenname)
{
    if (fullname.size() <= 1) { surname = fullname; givenname = L""; return; }
    // 默认单字姓
    surname = fullname.substr(0, 1);
    givenname = fullname.substr(1);
}

std::wstring ReplaceStr(const std::wstring& S, const std::wstring& Srch, const std::wstring& Replace)
{
    std::wstring result = S;
    size_t pos = 0;
    while ((pos = result.find(Srch, pos)) != std::wstring::npos)
    {
        result.replace(pos, Srch.length(), Replace);
        pos += Replace.length();
    }
    return result;
}

// ==== Puzzle ====
void Puzzle() { /* TODO */ }

// ==== SelectList ====
int SelectList(int begintalknum, int amount)
{
    // TODO
    return 0;
}

// ==== SetScene ====
void SetScene() { /* TODO */ }
void chengesnowhill() { /* TODO */ }

// ==== 属性获取函数（同构模式） ====
static int GetRoleStat(int rnum, bool Equip, int TRole::*field)
{
    int val = RRole[rnum].*field;
    // TODO: 功体加成, 装备加成
    return val;
}

int GetRoleMedcine(int rnum, bool Equip) { return RRole[rnum].Medcine; /* TODO: full */ }
int GetRoleMedPoi(int rnum, bool Equip) { return RRole[rnum].MedPoi; }
int GetRoleUsePoi(int rnum, bool Equip) { return RRole[rnum].UsePoi; }
int GetRoleAttPoi(int rnum, bool Equip) { return RRole[rnum].Defence; /* TODO */ }
int GetRoleDefPoi(int rnum, bool Equip) { return RRole[rnum].Defence; }
int GetRoleFist(int rnum, bool Equip) { return RRole[rnum].Fist; }
int GetRoleSword(int rnum, bool Equip) { return RRole[rnum].Sword; }
int GetRoleKnife(int rnum, bool Equip) { return RRole[rnum].Knife; }
int GetRoleUnusual(int rnum, bool Equip) { return RRole[rnum].Unusual; }
int GetRoleHidWeapon(int rnum, bool Equip) { return RRole[rnum].HidWeapon; }
int GetRoleAttack(int rnum, bool Equip) { return RRole[rnum].Attack; }
int GetRoleDefence(int rnum, bool Equip) { return RRole[rnum].Defence; }
int GetRoleSpeed(int rnum, bool Equip) { return RRole[rnum].Speed; }

int CheckEquipSet(int e0, int e1, int e2, int e3) { return -1; /* TODO */ }

// ==== 功体系统 ====
void StudyGongti() { /* TODO */ }
void ShowStudyGongti(int menu, int menu2, int max) { /* TODO */ }
int  StadyGongtiMenu(int x, int y, int w) { return -1; /* TODO */ }
void GongtiLevelUp(int rnum, int mnum) { /* TODO */ }
void AddSkillPoint(int num) { /* TODO */ }
bool AddBattleStateToEquip() { return false; /* TODO */ }

// ==== 输入 ====
int InputAmount() { return 0; /* TODO */ }
bool EnterString(std::string& str, int x, int y, int w, int h) { return false; /* TODO */ }