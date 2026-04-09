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
    SData[list[0]][list[1]][list[3]][list[2]] = static_cast<int16_t>(list[4]);
    InitialScene();
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
    int xstep = Sx, ystep = Sy;
    int stepx = (x1 > x2) ? 1 : ((x1 < x2) ? -1 : 0);
    int stepy = (y1 > y2) ? 1 : ((y1 < y2) ? -1 : 0);
    while (xstep != x2 || ystep != y2)
    {
        if (xstep != x2) xstep += stepx;
        if (ystep != y2) ystep += stepy;
        DrawSceneWithoutRole(xstep, ystep);
        DrawRoleOnScene(xstep, ystep);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(GameSpeed * 3);
    }
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
    for (int pic = beginpic; pic <= endpic; pic++)
    {
        DData[CurScene][enum_][5] = static_cast<int16_t>(pic);
        DData[CurScene][enum_][6] = static_cast<int16_t>(pic);
        DData[CurScene][enum_][7] = static_cast<int16_t>(pic);
        UpdateScene(DData[CurScene][enum_][10], DData[CurScene][enum_][9], pic - 1, pic);
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(GameSpeed * 5);
    }
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
    int ex = DData[CurScene][CurEvent][10];
    int ey = DData[CurScene][CurEvent][9];
    SData[CurScene][3][ex][ey] = -1;
    findway(x2, y2);
    for (int i = 0; i < nowstep; i++)
    {
        Moveman(x1, y1, linex[i], liney[i]);
        x1 = linex[i]; y1 = liney[i];
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(GameSpeed * 3);
    }
    SData[CurScene][3][ex][ey] = static_cast<int16_t>(CurEvent);
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
    int step = std::max(endpic1 - beginpic1, endpic2 - beginpic2);
    for (int i = 0; i <= step; i++)
    {
        int pic1 = std::min(beginpic1 + i, endpic1);
        int pic2 = std::min(beginpic2 + i, endpic2);
        DData[CurScene][enum1][5] = static_cast<int16_t>(pic1);
        DData[CurScene][enum1][6] = static_cast<int16_t>(pic1);
        DData[CurScene][enum1][7] = static_cast<int16_t>(pic1);
        DData[CurScene][enum2][5] = static_cast<int16_t>(pic2);
        DData[CurScene][enum2][6] = static_cast<int16_t>(pic2);
        DData[CurScene][enum2][7] = static_cast<int16_t>(pic2);
        UpdateScene(DData[CurScene][enum1][10], DData[CurScene][enum1][9],
            (i > 0) ? pic1 - 1 : 0, pic1);
        UpdateScene(DData[CurScene][enum2][10], DData[CurScene][enum2][9],
            (i > 0) ? pic2 - 1 : 0, pic2);
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(GameSpeed * 5);
    }
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
    // 判断5物品齐全逻辑 (code > 128)
    for (int i = 0; i < 5; i++)
    {
        if (list[i] > 0 && instruct_18(list[i], 0, 1) == 1)
            return list[6]; // 缺少物品
    }
    return list[5]; // 5物品齐全
}

void instruct_51()
{
    instruct_1(SOFTSTAR_BEGIN_TALK + rand() % SOFTSTAR_NUM_TALK, 0x72, 0);
}
void instruct_52()
{
    std::wstring str = L" 品德指數";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 75, CENTER_Y - 50, 150,
        ColColor(0x21), ColColor(0x23));
    str = std::format(L" {:5d}", static_cast<int>(RRole[0].Ethics));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 15, CENTER_Y - 25, ColColor(0x5), ColColor(0x7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    Redraw();
}
void instruct_53()
{
    std::wstring str = L" 聲望指數";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 75, CENTER_Y - 50, 150,
        ColColor(0x21), ColColor(0x23));
    str = std::format(L" {:5d}", static_cast<int>(RRole[0].Repute));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 15, CENTER_Y - 25, ColColor(0x5), ColColor(0x7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    Redraw();
}
void instruct_54()
{
    for (int i = 0; i < static_cast<int>(RScene.size()); i++)
    {
        if (RScene[i].EnCondition > 0)
            RScene[i].EnCondition = 0;
        if (RScene[i].EnCondition == -1)
            RScene[i].EnCondition = 2;
    }
}

int instruct_55(int enum_, int Value, int jump1, int jump2)
{
    return (DData[CurScene][enum_][0] == Value) ? jump1 : jump2;
}

void instruct_56(int Repute)
{
    RRole[0].Repute += static_cast<int16_t>(Repute);
    if (RRole[0].Repute > 200) RRole[0].Repute = 200;
}
void instruct_58()
{
    for (int i = 0; i < 15; i++)
    {
        int battlenum = 50 + rand() % 5;
        Battle(battlenum, 0);
        if ((i + 1) % 3 == 0)
        {
            // 每3场休息
            for (int j = 0; j < 6; j++)
            {
                int rnum = TeamList[j];
                if (rnum >= 0)
                {
                    RRole[rnum].CurrentHP = RRole[rnum].MaxHP;
                    RRole[rnum].CurrentMP = RRole[rnum].MaxMP;
                    RRole[rnum].PhyPower = MAX_PHYSICAL_POWER;
                }
            }
        }
    }
}

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

void instruct_62()
{
    Where = 3;
    EndAmi();
    LoadR(0);
    gametime += 1;
    Start();
}
void EndAmi()
{
    DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    int x = CENTER_X - Maker_Pic.pic->w / 2;
    int total = Maker_Pic.pic->h;
    for (int y = screen->h; y > -total; y -= 1)
    {
        DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 0);
        display_imgFromSurface(Maker_Pic, x, y);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(30);
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_EVENT_KEY_UP || ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                y = -total; // 跳过
                break;
            }
        }
    }
}
void instruct_63(int rnum, int sexual) { RRole[rnum].Sexual = static_cast<int16_t>(sexual); }
void instruct_64() { /* 空实现 — Pascal原始代码为空 */ }
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
        int px = e_GetValue(0, e1, e2);
        int py = e_GetValue(1, e1, e3);
        int addr = e_GetValue(2, e1, e4);
        uint32 c1 = ColColor(0, e_GetValue(3, e1, e5));
        uint32 c2 = ColColor(0, e_GetValue(4, e1, e6));
        const char* gbk = reinterpret_cast<const char*>(&x50[addr]);
        DrawGBKShadowText(gbk, px, py, c1, c2);
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
                break;
            }
        }
        break;
    }
    default:
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
    int16_t oldGongti = RRole[rnum].Gongti;
    if (oldGongti > -1)
    {
        int oldlv = GetGongtiLevel(rnum, oldGongti);
        RRole[rnum].MaxHP -= RMagic[oldGongti].AddHP[oldlv];
        RRole[rnum].MaxMP -= RMagic[oldGongti].AddMP[oldlv];
    }
    RRole[rnum].Gongti = static_cast<int16_t>(mnum);
    if (mnum > -1)
    {
        int newlv = GetGongtiLevel(rnum, mnum);
        RRole[rnum].MaxHP += RMagic[mnum].AddHP[newlv];
        RRole[rnum].MaxMP += RMagic[mnum].AddMP[newlv];
    }
    if (RRole[rnum].CurrentHP > RRole[rnum].MaxHP)
        RRole[rnum].CurrentHP = RRole[rnum].MaxHP;
    if (RRole[rnum].CurrentMP > RRole[rnum].MaxMP)
        RRole[rnum].CurrentMP = RRole[rnum].MaxMP;
    RRole[rnum].CurrentHP = std::max<int16_t>(RRole[rnum].CurrentHP, 0);
    RRole[rnum].CurrentMP = std::max<int16_t>(RRole[rnum].CurrentMP, 0);
}

// ==== GetGongtiState ====
bool GetGongtiState(int person, int state)
{
    if (RRole[person].Gongti <= -1) return false;
    int mnum = RRole[person].Gongti;
    if (RMagic[mnum].BattleState != state) return false;
    int lv = GetGongtiLevel(person, mnum);
    if (lv < RMagic[mnum].MaxLevel) return false;
    // 检查队伍中是否有人拥有petskill 5并在附近 (针对特定state)
    for (int i = 0; i < 6; i++)
    {
        int tm = TeamList[i];
        if (tm >= 0 && tm != person)
        {
            if (GetPetSkill(tm, 5) || GetPetSkill(tm, 4))
                return true;
        }
    }
    return true;
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
    if (skill < 0 || skill >= 10) return false;
    return RRole[rnum].Magic[skill] > 0;
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
    // 简化实现：读取对话并用 instruct_1 风格显示
    // 复杂的逐字GBK渲染用 NewTalk 实现
    FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
    FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return; }

    int offset = 0, len = 0;
    if (talknum == 0) { fread(&len, 4, 1, idx); }
    else { fseek(idx, (talknum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
    len -= offset;
    std::vector<uint8_t> buf(len + 2);
    fseek(grp, offset, SEEK_SET);
    fread(buf.data(), 1, len, grp);
    fclose(idx); fclose(grp);
    for (int i = 0; i < len; i++) { buf[i] ^= 0xFF; if (buf[i] == 0xFF) buf[i] = 0; }
    buf[len] = 0;

    // 读取名称
    std::wstring NameStr;
    if (namenum > 0)
    {
        FILE* nidx = PasOpen(AppPath + NAME_IDX, "rb");
        FILE* ngrp = PasOpen(AppPath + NAME_GRP, "rb");
        if (nidx && ngrp)
        {
            int noffset = 0, nlen = 0;
            if (namenum == 0) { fread(&nlen, 4, 1, nidx); }
            else { fseek(nidx, (namenum - 1) * 4, SEEK_SET); fread(&noffset, 4, 1, nidx); fread(&nlen, 4, 1, nidx); }
            nlen -= noffset;
            std::vector<uint8_t> namebuf(nlen + 2);
            fseek(ngrp, noffset, SEEK_SET);
            fread(namebuf.data(), 1, nlen, ngrp);
            for (int i = 0; i < nlen; i++) namebuf[i] ^= 0xFF;
            namebuf[nlen] = 0;
            NameStr = GBKToUnicode(reinterpret_cast<const char*>(namebuf.data()));
            fclose(nidx); fclose(ngrp);
        }
        else { if (nidx) fclose(nidx); if (ngrp) fclose(ngrp); }
    }
    else if (namenum == -2)
    {
        int HeadNumR = headnum;
        if (headnum >= 412 && headnum <= 429) HeadNumR = 0;
        for (int i = 0; i < static_cast<int>(RRole.size()); i++)
        {
            if (RRole[i].HeadNum == HeadNumR || (i == 0 && HeadNumR == 0))
            {
                NameStr = GBKToUnicode(reinterpret_cast<const char*>(RRole[i].Name));
                break;
            }
        }
    }

    std::wstring TalkStr = GBKToUnicode(reinterpret_cast<const char*>(buf.data()));
    // 名字替换
    std::wstring FullNameStr = GBKToUnicode(reinterpret_cast<const char*>(RRole[0].Name));
    TalkStr = ReplaceStr(TalkStr, L"&&", FullNameStr);
    std::wstring SurName, GivenName;
    DivideName(FullNameStr, SurName, GivenName);
    TalkStr = ReplaceStr(TalkStr, L"$$", SurName);
    TalkStr = ReplaceStr(TalkStr, L"%%", GivenName);

    // 去掉控制码，简单显示
    TalkStr = ReplaceStr(TalkStr, L"@@", L"");
    TalkStr = ReplaceStr(TalkStr, L"##", L"");
    TalkStr = ReplaceStr(TalkStr, L"**", L"\n");

    // 计算位置
    int Frame_X = 50;
    int Frame_Y = CENTER_Y * 2 - 180;
    int Talk_X = Frame_X + 50;
    int Talk_Y = Frame_Y + 35;
    int Head_X = 30, Head_Y = CENTER_Y * 2 - 120;

    if (place > 2) place = 5 - place;
    if (place == 1) { Head_X = CENTER_X * 2 - 100; Head_Y = CENTER_Y * 2 - 120; Talk_X = 30; }
    else if (place == 2) { Talk_X = Frame_X + 70; }

    Redraw();
    DrawRectangleWithoutFrame(0, Frame_Y, CENTER_X * 2, 170, 0, 60);
    if (showhead == 0 && headnum >= 0) DrawHeadPic(headnum, Head_X, Head_Y);
    if (!NameStr.empty())
    {
        DrawShadowText(reinterpret_cast<uint16_t*>(NameStr.data()),
            Talk_X, Frame_Y + 7, ColColor(5), ColColor(7));
    }

    // 逐行显示
    int row = 0, MaxRow = 5, MaxCol = 30;
    MaxCol = static_cast<int>((CENTER_X * 2 - (768 - MaxCol * 20)) / 20);
    int ColSpacing = 20, RowSpacing = 25;
    size_t pos = 0;
    while (pos < TalkStr.size())
    {
        int col = 0;
        while (pos < TalkStr.size() && TalkStr[pos] != L'\n' && col < MaxCol)
        {
            wchar_t ch = TalkStr[pos];
            if (ch == L'^' && pos + 1 < TalkStr.size()) { pos += 2; continue; } // 跳过颜色码
            std::wstring tmp(1, ch);
            tmp += L'\0';
            int xtemp = Talk_X + ColSpacing * col;
            if (static_cast<uint16_t>(ch) < 0x1000) xtemp += 5;
            DrawShadowText(reinterpret_cast<uint16_t*>(tmp.data()), xtemp, Talk_Y + RowSpacing * row,
                ColColor(0x6F63 & 0xFF), ColColor((0x6F63 >> 8) & 0xFF));
            col++;
            pos++;
        }
        if (pos < TalkStr.size() && TalkStr[pos] == L'\n') pos++;
        row++;
        if (row >= MaxRow)
        {
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            WaitAnyKey();
            row = 0;
            Redraw();
            DrawRectangleWithoutFrame(0, Frame_Y, CENTER_X * 2, 170, 0, 60);
            if (showhead == 0 && headnum >= 0) DrawHeadPic(headnum, Head_X, Head_Y);
            if (!NameStr.empty())
                DrawShadowText(reinterpret_cast<uint16_t*>(NameStr.data()),
                    Talk_X, Frame_Y + 7, ColColor(5), ColColor(7));
        }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    Redraw();
}

// ==== NewTalk ====
void NewTalk(int headnum, int talknum, int namenum, int place, int showhead, int color, int frame,
    const std::wstring& content, const std::wstring& disname)
{
    const int RowSpacing = 25, ColSpacing = 20, MaxRow = 5, NameColSpacing = 20;
    int MaxCol = 30;
    MaxCol = static_cast<int>((CENTER_X * 2 - (768 - MaxCol * ColSpacing)) / ColSpacing);
    int Frame_X = 50, Frame_Y = CENTER_Y * 2 - 180;
    int Talk_X = Frame_X + 50, Talk_Y = Frame_Y + 35;
    int Name_X = Talk_X, Name_Y = Frame_Y + 7;
    int Head_X = 30, Head_Y = CENTER_Y * 2 - 120;
    int Talk_W = MaxCol, Talk_H = MaxRow;

    if (place > 2) place = 5 - place;
    if (place == 0) { Head_X = 30; Head_Y = CENTER_Y * 2 - 120; }
    else if (place == 1) { Head_X = CENTER_X * 2 - 100; Head_Y = CENTER_Y * 2 - 120; Talk_X = 30; Name_X = Talk_X; Name_Y = Frame_Y + 7; }
    else if (place == 2) { Talk_X = Frame_X + 70; }

    // 颜色映射
    switch (color)
    {
    case 0: color = 28515; break;
    case 1: color = 28421; break;
    case 2: color = 28435; break;
    case 3: color = 28563; break;
    case 4: color = 28466; break;
    case 5: color = 28450; break;
    }
    uint8_t ForeGroundCol = color & 0xFF;
    uint8_t BackGroundCol = (color >> 8) & 0xFF;

    // 获取对话内容
    std::wstring TalkStr;
    if (content.empty())
    {
        if (talknum >= 0)
        {
            TalkStr = ReadTalk(talknum);
            if (!TalkStr.empty()) TalkStr = TalkStr.substr(1); // 去掉首字符
        }
        else
        {
            int idx = -talknum;
            if (idx >= 0 && idx < 0x10000)
            {
                const wchar_t* wp = reinterpret_cast<const wchar_t*>(&x50[idx]);
                TalkStr = wp;
            }
        }
    }
    else
        TalkStr = content;
    TalkStr = L" " + TalkStr;

    // 获取名称
    std::wstring NameStr;
    if (disname.empty())
    {
        if (namenum > 0)
        {
            FILE* nidx = PasOpen(AppPath + NAME_IDX, "rb");
            FILE* ngrp = PasOpen(AppPath + NAME_GRP, "rb");
            if (nidx && ngrp)
            {
                int noffset = 0, nlen = 0;
                fseek(nidx, (namenum - 1) * 4, SEEK_SET);
                fread(&noffset, 4, 1, nidx);
                fread(&nlen, 4, 1, nidx);
                nlen -= noffset;
                std::vector<uint8_t> namebuf(nlen + 2);
                fseek(ngrp, noffset, SEEK_SET);
                fread(namebuf.data(), 1, nlen, ngrp);
                for (int i = 0; i < nlen; i++) namebuf[i] ^= 0xFF;
                namebuf[nlen] = 0;
                NameStr = GBKToUnicode(reinterpret_cast<const char*>(namebuf.data()));
                fclose(nidx); fclose(ngrp);
            }
            else { if (nidx) fclose(nidx); if (ngrp) fclose(ngrp); }
        }
        else if (namenum == -2)
        {
            int HeadNumR = headnum;
            if (headnum >= 412 && headnum <= 429) HeadNumR = 0;
            for (int i = 0; i < static_cast<int>(RRole.size()); i++)
            {
                if (RRole[i].HeadNum == HeadNumR || (i == 0 && HeadNumR == 0))
                {
                    NameStr = GBKToUnicode(reinterpret_cast<const char*>(RRole[i].Name));
                    break;
                }
            }
        }
        else if (namenum == 0)
        {
            NameStr = GBKToUnicode(reinterpret_cast<const char*>(RRole[0].Name));
        }
    }
    else
        NameStr = disname;

    // 名字替换
    std::wstring FullNameStr = GBKToUnicode(reinterpret_cast<const char*>(RRole[0].Name));
    TalkStr = ReplaceStr(TalkStr, L"&&", FullNameStr);
    std::wstring SurName, GivenName;
    if (TalkStr.find(L"$$") != std::wstring::npos || TalkStr.find(L"%%") != std::wstring::npos)
    {
        DivideName(FullNameStr, SurName, GivenName);
        TalkStr = ReplaceStr(TalkStr, L"$$", SurName);
        TalkStr = ReplaceStr(TalkStr, L"%%", GivenName);
    }

    Redraw();

    uint32 DrawForeGroundCol = ColColor(ForeGroundCol);
    uint32 DrawBackGroundCol = ColColor(BackGroundCol);
    int len = static_cast<int>(TalkStr.size());
    int I = 0;
    bool skipSync = false;

    while (true)
    {
        Redraw();
        DrawRectangleWithoutFrame(0, Frame_Y, CENTER_X * 2, 170, 0, 60);
        if (showhead == 0 && headnum >= 0)
            DrawHeadPic(headnum, Head_X, Head_Y);
        if (!NameStr.empty() || showhead != 0)
        {
            for (int ix = 0; ix < static_cast<int>(NameStr.size()); ix++)
            {
                std::wstring tmp(1, NameStr[ix]);
                tmp += L'\0';
                DrawShadowText(reinterpret_cast<uint16_t*>(tmp.data()),
                    Name_X + ix * NameColSpacing, Name_Y, ColColor(5), ColColor(7));
            }
        }
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

        int ix = 0, iy = 0;
        skipSync = false;
        SDL_Event ev;
        while (SDL_PollEvent(&ev) || true)
        {
            CheckBasicEvent();
            if (ev.type == SDL_EVENT_KEY_UP && (ev.key.key == SDLK_ESCAPE))
            {
                skipSync = true;
                break;
            }
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && ev.button.button == SDL_BUTTON_RIGHT)
            {
                skipSync = true;
                break;
            }
            if (ev.type == SDL_EVENT_KEY_UP && (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_SPACE))
                skipSync = true;
            if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && ev.button.button == SDL_BUTTON_LEFT)
                skipSync = true;

            if (!(ix < Talk_W && iy < Talk_H && I < len))
                break;

            // 控制码检查: @@
            if (I + 1 < len && TalkStr[I] == L'@' && TalkStr[I + 1] == L'@')
            {
                I += 2;
                WaitAnyKey();
                continue;
            }
            // ##
            if (I + 1 < len && TalkStr[I] == L'#' && TalkStr[I + 1] == L'#')
            {
                I += 2;
                SDL_Delay(500);
                continue;
            }
            // **
            if (I + 1 < len && TalkStr[I] == L'*' && TalkStr[I + 1] == L'*')
            {
                iy++;
                ix = 0;
                I += 2;
                if (iy >= Talk_H)
                {
                    if (I < len) WaitAnyKey();
                    break;
                }
                continue;
            }
            // ^0 ~ ^5 颜色变换
            if (I + 1 < len && TalkStr[I] == L'^')
            {
                wchar_t nc = TalkStr[I + 1];
                if (nc >= L'0' && nc <= L'5')
                {
                    DrawBackGroundCol = ColColor(0x6F);
                    switch (nc - L'0')
                    {
                    case 0: DrawForeGroundCol = ColColor(0x63); break;
                    case 1: DrawForeGroundCol = ColColor(0x05); break;
                    case 2: DrawForeGroundCol = ColColor(0x13); break;
                    case 3: DrawForeGroundCol = ColColor(0x93); break;
                    case 4: DrawForeGroundCol = ColColor(0x32); break;
                    case 5: DrawForeGroundCol = ColColor(0x22); break;
                    }
                    I += 2;
                    continue;
                }
                // ^^ 恢复颜色
                if (nc == L'^')
                {
                    DrawBackGroundCol = ColColor(BackGroundCol);
                    DrawForeGroundCol = ColColor(ForeGroundCol);
                    I += 2;
                    continue;
                }
            }

            if (I < len)
            {
                std::wstring tmp(1, TalkStr[I]);
                tmp += L'\0';
                int xtemp = Talk_X + ColSpacing * ix;
                if (static_cast<uint16_t>(TalkStr[I]) < 0x1000) xtemp += 5;
                DrawShadowText(reinterpret_cast<uint16_t*>(tmp.data()),
                    xtemp, Talk_Y + RowSpacing * iy, DrawForeGroundCol, DrawBackGroundCol);
            }
            I++;
            if (!skipSync)
            {
                SDL_Delay(5);
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            ix++;
            if (ix >= Talk_W || iy >= Talk_H)
            {
                ix = 0;
                iy++;
                if (iy >= Talk_H)
                {
                    if (I <= len)
                    {
                        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                        WaitAnyKey();
                        if (skipSync) WaitAnyKey();
                        skipSync = false;
                    }
                    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                    break;
                }
            }
        }
        if (I >= len) break;
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    if (skipSync) WaitAnyKey();
    Redraw();
}

// ==== ShowTitle ====
void ShowTitle(int talknum, int color)
{
    int color1 = color & 0xFF;
    int color2 = (color >> 8) & 0xFF;
    int y = (Where != 3) ? 30 : 60;
    int w = 640, h = 109;
    int row = (Where != 3) ? 5 : 15;
    int cell = 25;

    FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
    FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return; }

    int offset = 0, len = 0;
    if (talknum == 0) { fread(&len, 4, 1, idx); }
    else { fseek(idx, (talknum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
    len -= offset;
    std::vector<uint8_t> talkarray(len + 2);
    fseek(grp, offset, SEEK_SET);
    fread(talkarray.data(), 1, len, grp);
    fclose(idx); fclose(grp);
    for (int i = 0; i < len; i++)
    {
        talkarray[i] ^= 0xFF;
        if (talkarray[i] == 0xFF) talkarray[i] = 0;
    }
    talkarray[len] = 0;

    // 名字数据
    const char* ap = reinterpret_cast<const char*>(RRole[0].Name);
    int alen = static_cast<int>(strlen(ap));

    const uint8_t* tp = talkarray.data();
    int tplen = static_cast<int>(strlen(reinterpret_cast<const char*>(tp)));
    int x1 = (tplen > cell * 2) ? 300 - cell * 10 : 300 - tplen * 5;
    int y1 = ((tplen / 2) > cell * row) ? y + h / 2 - 50 : y + h / 2 - 10 - ((tplen / 2) / cell) * 10;

    int ch = 0;
    uint16_t pword[2] = { 0, 0 };
    while (true)
    {
        pword[0] = *reinterpret_cast<const uint16_t*>(tp + ch);
        if ((pword[0] >> 8) == 0 || (pword[0] & 0xFF) == 0) break;

        if (Where != 3) Redraw();
        else DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 0);

        int c1 = 0, r1 = 0;
        DrawRectangleWithoutFrame(0, y, w, h, 0, 40);

        while (r1 < row)
        {
            pword[0] = *reinterpret_cast<const uint16_t*>(tp + ch);
            if ((pword[0] >> 8) == 0 || (pword[0] & 0xFF) == 0) break;
            ch += 2;

            if ((pword[0] & 0xFF) == 0x5E) // ^X 颜色码
            {
                int cv = ((pword[0] >> 8) & 0xFF) - 0x30;
                int newcolor;
                switch (cv)
                {
                case 0: newcolor = 28515; break;
                case 1: newcolor = 28421; break;
                case 2: newcolor = 28435; break;
                case 3: newcolor = 28563; break;
                case 4: newcolor = 28466; break;
                case 5: newcolor = 28450; break;
                case 64: newcolor = color; break;
                default: newcolor = color; break;
                }
                color1 = newcolor & 0xFF;
                color2 = (newcolor >> 8) & 0xFF;
            }
            else if (pword[0] == 0x2323) // ##
            {
                SDL_Delay(500 * GameSpeed / 10);
            }
            else if (pword[0] == 0x2A2A) // **
            {
                if (c1 > 0) { r1++; DrawRectangleWithoutFrame(0, y + h + 11 * (r1 - 1) + 1, w, 10, 0, 40); }
                c1 = 0;
            }
            else if (pword[0] == 0x4040) // @@
            {
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                WaitAnyKey();
            }
            else if (pword[0] == 0x2626 || pword[0] == 0x2525 || pword[0] == 0x2424) // && $$ %%
            {
                const char* np3 = ap; // 默认全名
                // 简化：仅用全名替代
                int ni = 0;
                while (true)
                {
                    uint16_t nw = *reinterpret_cast<const uint16_t*>(np3 + ni);
                    if ((nw >> 8) == 0 || (nw & 0xFF) == 0) break;
                    pword[0] = nw;
                    ni += 2;
                    DrawGBKShadowText(reinterpret_cast<const char*>(pword),
                        x1 + CHINESE_FONT_SIZE * c1, y1 + CHINESE_FONT_SIZE * r1,
                        ColColor(0, color1), ColColor(0, color2));
                    c1++;
                    if (c1 == cell) { c1 = 0; r1++; }
                }
            }
            else
            {
                DrawGBKShadowText(reinterpret_cast<const char*>(pword),
                    x1 + CHINESE_FONT_SIZE * c1, y1 + CHINESE_FONT_SIZE * r1,
                    ColColor(0, color1), ColColor(0, color2));
                c1++;
                if (c1 == cell) { c1 = 0; r1++; }
            }
        }
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        WaitAnyKey();
        if ((pword[0] & 0xFF) == 0 || (pword[0] >> 8) == 0) break;
    }
    if (Where != 3) Redraw();
    else DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 0);
}

// ==== ReSetName ====
int ReSetName(int t, int inum, int newnamenum)
{
    FILE* idx = PasOpen(AppPath + NAME_IDX, "rb");
    FILE* grp = PasOpen(AppPath + NAME_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return 0; }

    int offset = 0, len = 0;
    if (newnamenum == 0) { fread(&len, 4, 1, idx); }
    else { fseek(idx, (newnamenum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
    len -= offset;
    std::vector<uint8_t> talkarray(len + 2);
    fseek(grp, offset, SEEK_SET);
    fread(talkarray.data(), 1, len, grp);
    fclose(idx); fclose(grp);
    for (int i = 0; i < len; i++)
    {
        talkarray[i] ^= 0xFF;
        if (talkarray[i] == 0x2A) talkarray[i] = 0;
        if (talkarray[i] == 0xFF) talkarray[i] = 0;
    }
    talkarray[len] = 0;

    char* p = nullptr;
    switch (t)
    {
    case 0: p = reinterpret_cast<char*>(RRole[inum].Name); break;
    case 1: p = reinterpret_cast<char*>(RItem[inum].Name); break;
    case 2: p = reinterpret_cast<char*>(RScene[inum].Name); break;
    case 3: p = reinterpret_cast<char*>(RMagic[inum].Name); break;
    case 4: p = reinterpret_cast<char*>(RItem[inum].Introduction); break;
    default: return 0;
    }
    for (int i = 0; i <= len; i++)
        p[i] = static_cast<char>(talkarray[i]);
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
void Puzzle()
{
    int Y1 = Sy, X1 = Sx;
    if (SFace == 0) { Y1 = Sy; X1 = Sx - 1; }
    else if (SFace == 1) { Y1 = Sy + 1; X1 = Sx; }
    else if (SFace == 2) { Y1 = Sy - 1; X1 = Sx; }
    else if (SFace == 3) { Y1 = Sy; X1 = Sx + 1; }

    int CurDNum = SData[CurScene][3][X1][Y1];
    int CenterXY = DData[CurScene][CurEvent][8];
    int CenterY = CenterXY / 100;
    int CenterX = CenterXY % 100;
    if (CenterY == Sy || CenterX == Sx) return;

    int X221 = Y1 - CenterY;
    int X222 = X1 - CenterX;
    int X223 = Y1 - Sy;
    int X224 = X1 - Sx;
    int X245 = X221 * X224 - X222 * X223;

    int Array1000[9], Array1050[4];
    for (int i = 0; i < 9; i++) Array1000[i] = -1;
    for (int i = 0; i < 4; i++) Array1050[i] = -1;

    for (int x246 = 0; x246 < 4; x246++)
    {
        int dx = 0, dy = 0;
        if (x246 == 0) { X221 = 0; X222 = -1; }
        if (x246 == 1) { X221 = 1; X222 = 0; }
        if (x246 == 2) { X221 = 0; X222 = 1; }
        if (x246 == 3) { X221 = -1; X222 = 0; }
        int X207 = CenterY + X221;
        int X208 = CenterX + X222;
        int X258 = SData[CurScene][3][X208][X207];
        if (X258 != -1)
        {
            int X267 = DData[CurScene][X258][8];
            if (X267 == CenterXY)
            {
                Array1050[x246] = X258;
                int X262 = (X221 - 1) * X222 * X245;
                int X263 = (X222 + 1) * X221 * X245;
                int X209 = X207 + X262;
                int X210 = X208 + X263;
                if (SData[CurScene][3][X210][X209] != -1) break;
                int X211 = CenterY + X262;
                int X212 = CenterX + X263;
                if (SData[CurScene][3][X212][X211] != -1) break;
                int X248 = X262 + 1;
                int X249 = X263 + 1;
                int X250 = X249 * 3 + X248;
                Array1000[X250] = X258;
            }
        }
    }

    for (int x246 = 0; x246 < 4; x246++)
    {
        int X262 = Array1050[x246];
        if (X262 != -1)
        {
            SData[CurScene][3][X1][Y1] = -1;
            int a = DData[CurScene][CurDNum][5];
            DData[CurScene][CurDNum][1] = static_cast<int16_t>(X262);
            for (int j = 2; j <= 8; j++) DData[CurScene][CurDNum][j] = (j == 8) ? -1 : 0;
            DData[CurScene][CurDNum][9] = -1;
            DData[CurScene][CurDNum][10] = -1;
            UpdateScene(X1, Y1, a, 0);
        }
    }

    for (int x246 = 0; x246 < 9; x246++)
    {
        int X251 = Array1000[x246];
        if (X251 != -1)
        {
            int X266 = x246 + 3635;
            int X248 = x246 % 3;
            int X249 = x246 / 3;
            int X253 = X248 - 1;
            int X254 = X249 - 1;
            int X255 = CenterY + X253;
            int X256 = CenterX + X254;
            SData[CurScene][3][X256][X255] = static_cast<int16_t>(X251);
            DData[CurScene][X251][0] = 1;
            DData[CurScene][X251][2] = 540;
            DData[CurScene][X251][5] = static_cast<int16_t>(X266);
            DData[CurScene][X251][6] = static_cast<int16_t>(X266);
            DData[CurScene][X251][7] = static_cast<int16_t>(X266);
            DData[CurScene][X251][8] = static_cast<int16_t>(CenterXY);
            DData[CurScene][X251][9] = static_cast<int16_t>(X255);
            DData[CurScene][X251][10] = static_cast<int16_t>(X256);
            UpdateScene(X1, Y1, 0, X266);
        }
    }

    int X259 = SData[CurScene][3][X1][Y1];
    if (X259 != -1)
    {
        Sy = Y1 + X223;
        Sx = X1 + X224;
    }
    else
    {
        Sy = Y1;
        Sx = X1;
    }
}

// ==== SelectList ====
int SelectList(int begintalknum, int amount)
{
    int w = 0;
    MenuString.resize(amount);
    MenuEngString.resize(amount);
    FILE* idx = PasOpen(AppPath + TALK_IDX, "rb");
    FILE* grp = PasOpen(AppPath + TALK_GRP, "rb");
    if (!idx || !grp) { if (idx) fclose(idx); if (grp) fclose(grp); return 0; }

    for (int talknum = begintalknum; talknum < begintalknum + amount; talknum++)
    {
        int offset = 0, len = 0;
        if (talknum == 0) { fseek(idx, 0, SEEK_SET); fread(&len, 4, 1, idx); }
        else { fseek(idx, (talknum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fread(&len, 4, 1, idx); }
        len -= offset;
        std::vector<uint8_t> buf(len + 2);
        fseek(grp, offset, SEEK_SET);
        fread(buf.data(), 1, len, grp);
        for (int i = 0; i < len; i++) { buf[i] ^= 0xFF; if (buf[i] == 0xFF) buf[i] = 0; }
        buf[len] = 0;
        MenuString[talknum - begintalknum] = GBKToUnicode(reinterpret_cast<const char*>(buf.data()));
        MenuEngString[talknum - begintalknum] = L" ";
        if (len - 1 > w) w = len - 1;
    }
    int x = screen->w / 2 - w * 5 - 5;
    int y = 270 - amount * 22;
    int result = CommonMenu(x, y, w * 10 + 10, amount - 1);
    fclose(idx); fclose(grp);
    return result;
}

// ==== SetScene ====
void SetScene()
{
    Effect = 0; // Note: Pascal reads from Kys_ini, C++ simplified
    fog = false;
    rain = -1;
    Water = -1;
    snow = -1;
    showblackscreen = false;
    if (RScene[CurScene].Mapmode == 1)
    {
        for (int i1 = 0; i1 < 440; i1++)
            for (int i = 0; i < 640; i++)
            {
                int b = ((i - (screen->w >> 1)) * (i - (screen->w >> 1)) +
                    (i1 - (screen->h >> 1)) * (i1 - (screen->h >> 1))) / 150;
                if (b > 100) b = 100;
                snowalpha[i1][i] = static_cast<uint8_t>(b);
            }
        showblackscreen = true;
    }
    else if (Effect == 0)
    {
        if (RScene[CurScene].Mapmode == 2)
        {
            for (int i1 = 0; i1 <= 60; i1++)
            {
                int a = ((i1 / 15) % 2 == 0) ? 1 : -1;
                int b2 = std::abs((i1 % 15) / 5 - 2);
                int b = a * (b2 - 1);
                snowalpha[0][i1] = static_cast<uint8_t>(b);
            }
            Water = 0;
        }
        else if (RScene[CurScene].Mapmode == 3)
        {
            for (int i1 = 0; i1 < 440; i1++)
                for (int i = 0; i < 640; i++)
                {
                    int r = rand() % 170;
                    if (r == 0)
                    {
                        snowalpha[i1][i] = 1;
                        r = rand() % 10;
                        if (r == 0)
                        {
                            snowalpha[std::abs(i1 - 1)][i] = 1;
                            snowalpha[i1][std::abs(i - 1)] = 1;
                        }
                    }
                    else
                        snowalpha[i1][i] = 0;
                }
            snow = 0;
        }
        else if (RScene[CurScene].Mapmode == 4)
        {
            for (int i1 = 0; i1 < 440; i1++)
                for (int i = 0; i < 640; i++)
                    snowalpha[i1][i] = 0;
            for (int i1 = 0; i1 < 440; i1++)
                for (int i = 0; i < 640; i++)
                {
                    int r = rand() % 200;
                    if (r == 0)
                    {
                        snowalpha[i1][i] = 1;
                        r = rand() % 10;
                        for (int i2 = 0; i2 <= r; i2++)
                        {
                            int a = (i1 + i2);
                            if (a > 439) a -= 440;
                            snowalpha[a][i] = 1;
                        }
                    }
                }
            rain = 0;
        }
        else if (RScene[CurScene].Mapmode == 5)
        {
            for (int i1 = 0; i1 < 440; i1++)
                for (int i = 0; i < 640; i++)
                    snowalpha[i1][i] = static_cast<uint8_t>(60 + rand() % 10);
            fog = true;
        }
    }
}
void chengesnowhill()
{
    SData[CurScene][0][52][33] = 1220; SData[CurScene][4][52][33] = 32;
    SData[CurScene][0][52][32] = 1222; SData[CurScene][4][52][32] = 24;
    SData[CurScene][0][52][31] = 1224; SData[CurScene][4][52][31] = 16;
    SData[CurScene][0][52][30] = 1226; SData[CurScene][4][52][30] = 8;
    SData[CurScene][0][17][45] = 1220; SData[CurScene][4][17][45] = 32;
    SData[CurScene][0][16][45] = 1222; SData[CurScene][4][16][45] = 24;
    SData[CurScene][0][15][45] = 1224; SData[CurScene][4][15][45] = 16;
    SData[CurScene][0][14][45] = 1226; SData[CurScene][4][14][45] = 8;
    for (int i = 18; i <= 52; i++) { SData[CurScene][0][i][45] = 1216; SData[CurScene][4][i][45] = 36; }
    for (int i = 34; i <= 44; i++) { SData[CurScene][0][52][i] = 1216; SData[CurScene][4][52][i] = 36; }
    InitialScene();
    instruct_19(29, 52);
}

// ==== 属性获取函数（同构模式） ====
int GetRoleMedcine(int rnum, bool Equip)
{
    int result = RRole[rnum].Medcine;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddMedcine;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddMedcine;
    return result;
}

int GetRoleMedPoi(int rnum, bool Equip)
{
    int result = RRole[rnum].MedPoi;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddMedPoi;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddMedPoi;
    return result;
}

int GetRoleUsePoi(int rnum, bool Equip)
{
    int result = RRole[rnum].UsePoi;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddUsePoi;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddUsePoi;
    return result;
}

int GetRoleDefPoi(int rnum, bool Equip)
{
    int result = RRole[rnum].DefPoi;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddDefPoi;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddDefPoi;
    return result;
}

int GetRoleFist(int rnum, bool Equip)
{
    int result = RRole[rnum].Fist;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddFist;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddFist;
    return result;
}

int GetRoleSword(int rnum, bool Equip)
{
    int result = RRole[rnum].Sword;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddSword;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddSword;
    return result;
}

int GetRoleKnife(int rnum, bool Equip)
{
    int result = RRole[rnum].Knife;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddKnife;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddKnife;
    return result;
}

int GetRoleUnusual(int rnum, bool Equip)
{
    int result = RRole[rnum].Unusual;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddUnusual;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddUnusual;
    return result;
}

int GetRoleHidWeapon(int rnum, bool Equip)
{
    int result = RRole[rnum].HidWeapon;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        if (lv == RMagic[RRole[rnum].Gongti].MaxLevel)
            result += RMagic[RRole[rnum].Gongti].AddHidWeapon;
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddHidWeapon;
    return result;
}

int GetRoleAttack(int rnum, bool Equip)
{
    int result = RRole[rnum].Attack;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        result += RMagic[RRole[rnum].Gongti].AddAtt[lv];
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddAttack;
    return result;
}

int GetRoleDefence(int rnum, bool Equip)
{
    int result = RRole[rnum].Defence;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        result += RMagic[RRole[rnum].Gongti].AddDef[lv];
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddDefence;
    return result;
}

int GetRoleSpeed(int rnum, bool Equip)
{
    int result = RRole[rnum].Speed;
    if (RRole[rnum].Gongti > -1)
    {
        int lv = GetGongtiLevel(rnum, RRole[rnum].Gongti);
        result += RMagic[RRole[rnum].Gongti].AddSpd[lv];
    }
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddSpeed;
    return result;
}

int GetRoleAttPoi(int rnum, bool Equip)
{
    int result = RRole[rnum].AttPoi;
    if (Equip)
        for (int l = 0; l < 4; l++)
            if (RRole[rnum].Equip[l] >= 0)
                result += RItem[RRole[rnum].Equip[l]].AddAttPoi;
    return result;
}

int CheckEquipSet(int e0, int e1, int e2, int e3)
{
    int result = -1;
    for (int i = 0; i < 5; i++)
    {
        if (SetNum[i][0] != e0 && SetNum[i][0] >= 0) continue;
        if (SetNum[i][1] != e1 && SetNum[i][1] >= 0) continue;
        if (SetNum[i][2] != e2 && SetNum[i][2] >= 0) continue;
        if (SetNum[i][3] != e3 && SetNum[i][3] >= 0) continue;
        result = i;
    }
    return result;
}

// ==== 功体系统 ====
void StudyGongti()
{
    int x = 10, y = 10;
    int x1 = x + 110, y1 = y;
    int x2 = x1, y2 = y + 210;
    int w = 100, w1 = 510, w2 = 100;
    int h1 = 200, h2 = 210;
    int max1 = 0, max2 = 0;
    int teammenu = 0, magicmenu = -1, position = 0;
    std::vector<int> magic;

    for (int i = 0; i < static_cast<int>(sizeof(TeamList) / sizeof(TeamList[0])); i++)
    {
        if (TeamList[i] < 0) break;
        max1++;
        RRole[TeamList[i]].Moveable = 0;
    }
    int h = max1 * 22 + 10;

    // 初始化功体列表
    max2 = 0;
    magic.clear();
    for (int i = 0; i < 10; i++)
    {
        if (RRole[TeamList[teammenu]].Magic[i] > 0 && RMagic[RRole[TeamList[teammenu]].Magic[i]].MagicType == 5)
        {
            magic.push_back(i);
            max2++;
        }
    }

    Redraw();
    ShowStudyGongti(teammenu, magicmenu, max1);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    SDL_Event ev;
    ev.key.key = 0;
    while (SDL_WaitEvent(&ev))
    {
        CheckBasicEvent();
        if (ev.type == SDL_EVENT_KEY_UP)
        {
            if (ev.key.key == SDLK_UP)
            {
                if (position == 0) { teammenu--; if (teammenu < 0) teammenu = max1 - 1; }
                else { magicmenu--; if (magicmenu < 0) magicmenu = max2 - 1; }
            }
            if (ev.key.key == SDLK_DOWN)
            {
                if (position == 0) { teammenu++; if (teammenu >= max1) teammenu = 0; }
                else { magicmenu++; if (magicmenu >= max2) magicmenu = 0; }
            }
            if (ev.key.key == SDLK_ESCAPE)
            {
                if (position == 0) break;
                else { magicmenu = -1; position = 0; }
            }
            if (ev.key.key == SDLK_SPACE || ev.key.key == SDLK_RETURN)
            {
                if (position == 0)
                {
                    max2 = 0; magic.clear();
                    for (int i = 0; i < 10; i++)
                        if (RRole[TeamList[teammenu]].Magic[i] > 0 && RMagic[RRole[TeamList[teammenu]].Magic[i]].MagicType == 5)
                        { magic.push_back(i); max2++; }
                    if (max2 > 0) { position = 1; magicmenu = 0; }
                    else magicmenu = -1;
                }
                else
                {
                    int rnum = TeamList[teammenu];
                    int mnum = RRole[rnum].Magic[magic[magicmenu]];
                    int lv = GetGongtiLevel(rnum, mnum);
                    if (RMagic[mnum].MaxLevel > lv && RRole[rnum].GongtiExam >= static_cast<uint16_t>(RMagic[mnum].NeedExp[lv + 1]))
                    {
                        MenuString.resize(2);
                        MenuString[0] = L" 學習";
                        MenuString[1] = L" 取消";
                        if (StadyGongtiMenu(x2 + 300, y2 + 6, 98) == 0)
                        {
                            GongtiLevelUp(rnum, mnum);
                            RRole[rnum].GongtiExam -= RMagic[mnum].NeedExp[lv + 1];
                            RRole[rnum].MagLevel[magic[magicmenu]] += 100;
                            RRole[rnum].Moveable = 0;
                        }
                        MenuString.clear();
                    }
                }
            }
            Redraw();
            ShowStudyGongti(teammenu, magicmenu, max1);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (ev.button.button == SDL_BUTTON_RIGHT)
            {
                if (position == 0) break;
                else { magicmenu = -1; position = 0; }
            }
            if (ev.button.button == SDL_BUTTON_LEFT)
            {
                if (position == 0)
                {
                    max2 = 0; magic.clear();
                    for (int i = 0; i < 10; i++)
                        if (RRole[TeamList[teammenu]].Magic[i] > 0 && RMagic[RRole[TeamList[teammenu]].Magic[i]].MagicType == 5)
                        { magic.push_back(i); max2++; }
                    if (max2 > 0) { position = 1; magicmenu = 0; }
                    else magicmenu = -1;
                }
                else
                {
                    int rnum = TeamList[teammenu];
                    int mnum = RRole[rnum].Magic[magic[magicmenu]];
                    int lv = GetGongtiLevel(rnum, mnum);
                    if (RMagic[mnum].MaxLevel > lv && RRole[rnum].GongtiExam >= static_cast<uint16_t>(RMagic[mnum].NeedExp[lv + 1]))
                    {
                        MenuString.resize(2);
                        MenuString[0] = L" 學習";
                        MenuString[1] = L" 取消";
                        if (StadyGongtiMenu(x2 + 300, y2 + 6, 98) == 0)
                        {
                            GongtiLevelUp(rnum, mnum);
                            RRole[rnum].GongtiExam -= RMagic[mnum].NeedExp[lv + 1];
                            RRole[rnum].MagLevel[magic[magicmenu]] += 100;
                            RRole[rnum].Moveable = 0;
                        }
                        MenuString.clear();
                    }
                }
            }
            Redraw();
            ShowStudyGongti(teammenu, magicmenu, max1);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
    }
}
void ShowStudyGongti(int menu, int menu2, int max)
{
    int x = 10, y = 10;
    int x1 = x + 110, y1 = y;
    int x2 = x1, y2 = y + 210;
    int w = 100, w1 = 510, w2 = 110;
    int h1 = 200, h2 = 210;
    int h = max * 22 + 10;
    int x3 = x2 + 120, y3 = y2, w3 = w1 - 120, h3 = h2;

    DrawRectangle(x, y, w, h, 0, 0xFFFFFF, 60);
    DrawRectangle(x1, y1, w1, h1, 0, 0xFFFFFF, 60);

    for (int i = 0; i < max; i++)
    {
        std::wstring name = GBKToUnicode(reinterpret_cast<const char*>(RRole[TeamList[i]].Name));
        if (menu != i)
            DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), x - 10, y + 5 + 22 * i, ColColor(0, 5), ColColor(0, 7));
        else
            DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), x - 10, y + 5 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
    }

    int rnum = TeamList[menu];
    if (rnum < 0) return;

    DrawHeadPic(RRole[rnum].HeadNum, x1 + 23, y1 + 70);
    std::wstring pname = GBKToUnicode(reinterpret_cast<const char*>(RRole[rnum].Name));
    int l = static_cast<int>(pname.size()) - 1;
    DrawShadowText(reinterpret_cast<uint16_t*>(pname.data()), x1 + 30 - l * 10, y1 + 78, ColColor(0, 0x5), ColColor(0, 0x7));

    auto drawStatStr = [&](const wchar_t* label, int val, int px, int py) {
        std::wstring s = std::format(L"{}", val);
        DrawShadowText(reinterpret_cast<uint16_t*>(s.data()), px + 70, py, ColColor(0, 0x5), ColColor(0, 0x7));
        std::wstring ls(label);
        DrawShadowText(reinterpret_cast<uint16_t*>(ls.data()), px, py, ColColor(0, 0x5), ColColor(0, 0x7));
    };

    std::wstring str = std::format(L"{}", static_cast<int>(RRole[rnum].Level));
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x1 + 70, y1 + 103, ColColor(0, 0x5), ColColor(0, 0x7));
    str = L" 等級";
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x1, y1 + 103, ColColor(0, 0x5), ColColor(0, 0x7));

    UpdateHpMp(rnum, x1, y1 + 107);

    drawStatStr(L" 攻擊", GetRoleAttack(rnum, false), x1 + 90, y1 + 10);
    drawStatStr(L" 防禦", GetRoleDefence(rnum, false), x1 + 90, y1 + 32);
    drawStatStr(L" 輕功", GetRoleSpeed(rnum, false), x1 + 90, y1 + 54);
    drawStatStr(L" 醫療", GetRoleMedcine(rnum, false), x1 + 200, y1 + 10);
    drawStatStr(L" 用毒", GetRoleUsePoi(rnum, false), x1 + 200, y1 + 32);
    drawStatStr(L" 解毒", GetRoleMedPoi(rnum, false), x1 + 200, y1 + 54);
    drawStatStr(L" 抗毒", GetRoleDefPoi(rnum, false), x1 + 200, y1 + 76);
    drawStatStr(L" 拳掌", GetRoleFist(rnum, false), x1 + 310, y1 + 10);
    drawStatStr(L" 禦劍", GetRoleSword(rnum, false), x1 + 310, y1 + 32);
    drawStatStr(L" 耍刀", GetRoleKnife(rnum, false), x1 + 310, y1 + 54);
    drawStatStr(L" 奇門", GetRoleUnusual(rnum, false), x1 + 310, y1 + 76);
    drawStatStr(L" 暗器", GetRoleHidWeapon(rnum, false), x1 + 310, y1 + 98);

    int max2 = 0;
    DrawRectangle(x2, y2, w2, h2, 0, 0xFFFFFF, 60);
    DrawRectangle(x3, y3, w3, h3, 0, 0xFFFFFF, 60);

    for (int i = 0; i < 10; i++)
    {
        if (RRole[rnum].Magic[i] > 0 && RMagic[RRole[rnum].Magic[i]].MagicType == 5)
        {
            std::wstring mname = GBKToUnicode(reinterpret_cast<const char*>(RMagic[RRole[rnum].Magic[i]].Name));
            if (menu2 != max2)
            {
                DrawShadowText(reinterpret_cast<uint16_t*>(mname.data()), x2 - 15, y2 + 5 + 22 * max2, ColColor(0, 5), ColColor(0, 7));
            }
            else
            {
                int mnum = RRole[rnum].Magic[i];
                int lv = GetGongtiLevel(rnum, mnum);
                int i1 = 0;
                DrawShadowText(reinterpret_cast<uint16_t*>(mname.data()), x2 - 15, y2 + 5 + 22 * max2, ColColor(0, 0x64), ColColor(0, 0x66));
                DrawShadowText(reinterpret_cast<uint16_t*>(mname.data()), x3 - 10, y3 + 5, ColColor(0, 0x5), ColColor(0, 0x7));

                const wchar_t* lvStr[] = { L" 目前等級   熟練", L" 目前等級   精純", L" 目前等級   化境" };
                str = lvStr[std::min(lv, 2)];
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 27, ColColor(0, 0x64), ColColor(0, 0x66));

                if (lv >= RMagic[mnum].MaxLevel)
                {
                    str = L" 已到達頂級";
                    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 49, ColColor(0, 0x21), ColColor(0, 0x23));
                }
                else
                {
                    str = L" 所需經驗值 ";
                    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 49, ColColor(0, 0x21), ColColor(0, 0x23));
                    str = std::format(L" {}", static_cast<int>(RMagic[mnum].NeedExp[lv + 1]));
                    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 + 103, y3 + 49, ColColor(0, 0x64), ColColor(0, 0x66));
                }

                str = L" 現有經驗值 ";
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 71, ColColor(0, 0x21), ColColor(0, 0x23));
                str = std::format(L" {}", static_cast<int>(RRole[rnum].GongtiExam));
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 + 103, y3 + 71, ColColor(0, 0x64), ColColor(0, 0x66));

                // 显示加成信息
                auto showBonus = [&](int val, const wchar_t* label) {
                    if (val != 0)
                    {
                        str = std::format(L"{}", val);
                        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10 + (i1 % 4) * 90 + 50, y3 - 5 + 98 + (i1 / 4) * 22, ColColor(0, 0x5), ColColor(0, 0x7));
                        str = label;
                        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10 + (i1 % 4) * 90, y3 - 5 + 98 + (i1 / 4) * 22, ColColor(0, 0x5), ColColor(0, 0x7));
                        i1++;
                    }
                };
                showBonus(RMagic[mnum].AddHP[lv], L" 生命");
                showBonus(RMagic[mnum].AddMP[lv], L" 內力");
                showBonus(RMagic[mnum].AddAtt[lv], L" 攻擊");
                showBonus(RMagic[mnum].AddDef[lv], L" 防禦");
                showBonus(RMagic[mnum].AddSpd[lv], L" 輕功");
                if (lv == RMagic[mnum].MaxLevel)
                {
                    showBonus(RMagic[mnum].AddMedcine, L" 醫療");
                    showBonus(RMagic[mnum].AddUsePoi, L" 用毒");
                    showBonus(RMagic[mnum].AddMedPoi, L" 解毒");
                    showBonus(RMagic[mnum].AddDefPoi, L" 抗毒");
                    showBonus(RMagic[mnum].AddFist, L" 拳掌");
                    showBonus(RMagic[mnum].AddSword, L" 禦劍");
                    showBonus(RMagic[mnum].AddKnife, L" 耍刀");
                    showBonus(RMagic[mnum].AddUnusual, L" 奇門");
                    showBonus(RMagic[mnum].AddHidWeapon, L" 暗器");
                }

                if (RMagic[mnum].BattleState > 0 && lv >= RMagic[mnum].MaxLevel)
                {
                    str = L" 功體特效 ";
                    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 115 + (i1 / 4) * 22, ColColor(0, 0x21), ColColor(0, 0x23));
                    const wchar_t* bsNames[] = { L"", L" 體力不減", L" 女性武功威力加成", L" 飲酒功效加倍",
                        L" 隨機傷害轉移", L" 隨機傷害反噬", L" 內傷免疫", L" 殺傷體力",
                        L" 增加閃躲幾率", L" 攻擊力隨等级循环增减", L" 內力消耗減少",
                        L" 每回合恢復生命", L" 負面狀態免疫", L" 全部武功威力加成",
                        L" 隨機二次攻擊", L" 拳掌武功威力加成", L" 劍術武功威力加成",
                        L" 刀法武功威力加成", L" 奇門武功威力加成", L" 增加內傷幾率",
                        L" 增加封穴幾率", L" 攻擊微量吸血", L" 攻擊距離增加",
                        L" 每回合恢復內力", L" 使用暗器距離增加", L" 附加殺傷吸收內力" };
                    int bsIdx = RMagic[mnum].BattleState;
                    if (bsIdx >= 1 && bsIdx <= 25)
                    {
                        str = bsNames[bsIdx];
                        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x3 - 10, y3 + 137 + (i1 / 4) * 22, ColColor(0, 0x64), ColColor(0, 0x66));
                    }
                }
            }
            max2++;
        }
    }
}
int StadyGongtiMenu(int x, int y, int w)
{
    int menu = 0;
    RegionRect.w = 1;
    RegionRect.h = 0;
    ShowCommonMenu2(x, y, w, menu);
    SDL_UpdateRect2(screen, x, y, w + 1, 29);
    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        CheckBasicEvent();
        if (ev.type == SDL_EVENT_KEY_DOWN)
        {
            if (ev.key.key == SDLK_LEFT || ev.key.key == SDLK_RIGHT)
            {
                menu = 1 - menu;
                ShowCommonMenu2(x, y, w, menu);
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
            }
        }
        if (ev.type == SDL_EVENT_KEY_UP)
        {
            if (ev.key.key == SDLK_ESCAPE && Where <= 2) { RegionRect.w = 0; return -1; }
            if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_SPACE) { RegionRect.w = 0; return menu; }
        }
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (ev.button.button == SDL_BUTTON_RIGHT && Where <= 2) { RegionRect.w = 0; return -1; }
            if (ev.button.button == SDL_BUTTON_LEFT) { RegionRect.w = 0; return menu; }
        }
        if (ev.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym;
            SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + 29)
            {
                int newmenu = (xm - x - 2) / 50;
                if (newmenu > 1) newmenu = 1;
                if (newmenu < 0) newmenu = 0;
                if (newmenu != menu)
                {
                    menu = newmenu;
                    ShowCommonMenu2(x, y, w, menu);
                    SDL_UpdateRect2(screen, x, y, w + 1, 29);
                }
            }
        }
    }
    RegionRect.w = 0;
    return -1;
}
void GongtiLevelUp(int rnum, int mnum)
{
    if (mnum != RRole[rnum].Gongti) return;
    int lv = GetGongtiLevel(rnum, mnum);
    RRole[rnum].CurrentHP -= RMagic[mnum].AddHP[lv];
    RRole[rnum].CurrentMP -= RMagic[mnum].AddMP[lv];
    RRole[rnum].MaxHP -= RMagic[mnum].AddHP[lv];
    RRole[rnum].MaxMP -= RMagic[mnum].AddMP[lv];
    RRole[rnum].CurrentHP += RMagic[mnum].AddHP[lv + 1];
    RRole[rnum].CurrentMP += RMagic[mnum].AddMP[lv + 1];
    RRole[rnum].MaxHP += RMagic[mnum].AddHP[lv + 1];
    RRole[rnum].MaxMP += RMagic[mnum].AddMP[lv + 1];
    RRole[rnum].CurrentHP = std::max<int16_t>(RRole[rnum].CurrentHP, 0);
    RRole[rnum].CurrentMP = std::max<int16_t>(RRole[rnum].CurrentMP, 0);
}
void AddSkillPoint(int num)
{
    RRole[0].AddSkillPoint += static_cast<int16_t>(num);
    DrawRectangle(CENTER_X - 75, 98, 145, 30, 0, ColColor(0, 255), 30);
    std::wstring word = L" 得到技能點  ";
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), CENTER_X - 90, 100, ColColor(0, 0x5), ColColor(0, 0x7));
    word = std::format(L"{}", num);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(word.data()), CENTER_X + 50, 100, ColColor(0, 0x64), ColColor(0, 0x66));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
    Redraw();
}
bool AddBattleStateToEquip()
{
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    const wchar_t* stateNames[] = {
        L"", L"體力不減", L"女性武功威力加成", L"飲酒功效加倍",
        L"隨機傷害轉移", L"隨機傷害反噬", L"內傷免疫", L"殺傷體力",
        L"增加閃躲幾率", L"攻擊力隨等级循环增减", L"內力消耗減少",
        L"每回合恢復生命", L"負面狀態免疫", L"全部武功威力加成",
        L"隨機二次攻擊", L"拳掌武功威力加成", L"劍術武功威力加成",
        L"刀法武功威力加成", L"奇門武功威力加成", L"增加內傷幾率",
        L"增加封穴幾率", L"攻擊微量吸血", L"攻擊距離增加",
        L"每回合恢復內力", L"使用暗器距離增加", L"附加殺傷吸收內力"
    };

    int state[26] = {};
    int n = 0;
    std::vector<int16_t> battlestate;

    for (int i = 0; i < 36; i++)
    {
        if (RRole[i].TeamState == 1 || RRole[i].TeamState == 2)
        {
            for (int i1 = 0; i1 < 10; i1++)
            {
                if (RRole[i].Magic[i1] > 0 && RMagic[RRole[i].Magic[i1]].MagicType == 5
                    && RMagic[RRole[i].Magic[i1]].BattleState > 0
                    && GetGongtiLevel(i, RRole[i].Magic[i1]) >= RMagic[RRole[i].Magic[i1]].MaxLevel)
                {
                    state[RMagic[RRole[i].Magic[i1]].BattleState] = 1;
                }
            }
        }
    }

    MenuString.clear();
    MenuEngString.clear();
    for (int i = 1; i <= 25; i++)
    {
        if (state[i] == 1)
        {
            n++;
            MenuString.push_back(stateNames[i]);
            battlestate.push_back(static_cast<int16_t>(i));
        }
    }
    if (n == 0)
    {
        Redraw();
        std::wstring str1 = L"沒有可用功體特效";
        DrawTextWithRect(reinterpret_cast<uint16_t*>(str1.data()), 320 - 85, 45, 170, ColColor(0x21), ColColor(0x23));
        WaitAnyKey();
        return false;
    }

    std::wstring title = L"選擇功體特效";
    int menu1 = TitleCommonScrollMenu(reinterpret_cast<uint16_t*>(title.data()),
        ColColor(0, 5), ColColor(0, 7), 5, 5, 300, n - 1, 17);
    if (menu1 >= 0)
    {
        int16_t SelectedState = battlestate[menu1];
        n = 0;
        MenuString.clear();
        MenuEngString.clear();
        std::vector<int16_t> EquipList;
        for (int i = 0; i < static_cast<int>(RItem.size()); i++)
        {
            if (GetItemCount(i) > 0 && RItem[i].ItemType == 1 && RItem[i].SetNum > 0 && RItem[i].BattleEffect <= 0)
            {
                n++;
                MenuString.push_back(GBKToUnicode(reinterpret_cast<const char*>(RItem[i].Name)));
                EquipList.push_back(static_cast<int16_t>(i));
            }
        }
        if (n == 0)
        {
            Redraw();
            std::wstring str1 = L"沒有可注入的裝備";
            DrawTextWithRect(reinterpret_cast<uint16_t*>(str1.data()), 320 - 85, 45, 170, ColColor(0x21), ColColor(0x23));
            WaitAnyKey();
            return false;
        }
        title = L"選擇裝備";
        int menu2 = TitleCommonScrollMenu(reinterpret_cast<uint16_t*>(title.data()),
            ColColor(0, 5), ColColor(0, 7), 315, 5, 300, n - 1, 17);
        if (menu2 >= 0)
        {
            RItem[EquipList[menu2]].BattleEffect = SelectedState;
            return true;
        }
    }
    return false;
}

// ==== 输入 ====
int InputAmount()
{
    int amount = 0;
    std::wstring str = L"輸入數字";
    std::wstring countstr = std::format(L"{:5d}", amount);
    DrawRectangle(CENTER_X - 100, CENTER_Y - 15, 200, 30, 0, ColColor(255), 100);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 100, CENTER_Y - 10, ColColor(5), ColColor(7));
    DrawEngText(screen, reinterpret_cast<uint16_t*>(countstr.data()), CENTER_X + 41, CENTER_Y - 10, ColColor(7));
    DrawEngText(screen, reinterpret_cast<uint16_t*>(countstr.data()), CENTER_X + 40, CENTER_Y - 10, ColColor(5));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        CheckBasicEvent();
        if (ev.type == SDL_EVENT_KEY_UP)
        {
            if (ev.key.key == SDLK_UP) amount += 10;
            if (ev.key.key == SDLK_DOWN) amount -= 10;
            if (ev.key.key == SDLK_LEFT) amount -= 1;
            if (ev.key.key == SDLK_RIGHT) amount += 1;
            if (amount > 32767) amount = 32767;
            if (amount < 0) amount = 0;
            if (ev.key.key >= SDLK_0 && ev.key.key <= SDLK_9)
            {
                if (amount < 3276) amount = amount * 10 + (ev.key.key - SDLK_0);
            }
            if (ev.key.key == SDLK_RETURN) break;
            if (ev.key.key == SDLK_BACKSPACE) amount /= 10;
        }
        countstr = std::format(L"{:5d}", amount);
        DrawRectangle(CENTER_X - 100, CENTER_Y - 15, 200, 30, 0, ColColor(255), 100);
        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), CENTER_X - 100, CENTER_Y - 10, ColColor(5), ColColor(7));
        DrawEngText(screen, reinterpret_cast<uint16_t*>(countstr.data()), CENTER_X + 41, CENTER_Y - 10, ColColor(7));
        DrawEngText(screen, reinterpret_cast<uint16_t*>(countstr.data()), CENTER_X + 40, CENTER_Y - 10, ColColor(5));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
    return amount;
}
bool EnterString(std::string& str, int x, int y, int w, int h)
{
    SDL_Rect r;
    r.x = x; r.y = y; r.w = w; r.h = h;
    SDL_StartTextInput(window);
    SDL_SetTextInputArea(window, &r, 0);
    while (true)
    {
        std::wstring dispStr = L" 請輸入名字：";
        std::wstring wstr;
        // Convert UTF-8 str to wstring for display
        for (size_t i = 0; i < str.size(); )
        {
            unsigned char c = str[i];
            wchar_t wc = 0;
            if (c < 0x80) { wc = c; i++; }
            else if ((c & 0xE0) == 0xC0 && i + 1 < str.size()) { wc = ((c & 0x1F) << 6) | (str[i + 1] & 0x3F); i += 2; }
            else if ((c & 0xF0) == 0xE0 && i + 2 < str.size()) { wc = ((c & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F); i += 3; }
            else { i++; continue; }
            wstr += wc;
        }
        dispStr += wstr;
        DrawTextWithRect(reinterpret_cast<uint16_t*>(dispStr.data()), x, y, 220, ColColor(0x66), ColColor(0x63));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Event ev;
        SDL_PollEvent(&ev);
        if (ev.type == SDL_EVENT_TEXT_INPUT)
        {
            str += ev.text.text;
        }
        if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && ev.button.button == SDL_BUTTON_RIGHT)
        {
            SDL_StopTextInput(window);
            return false;
        }
        if (ev.type == SDL_EVENT_KEY_UP)
        {
            if (ev.key.key == SDLK_RETURN) { SDL_StopTextInput(window); return true; }
            if (ev.key.key == SDLK_ESCAPE) { SDL_StopTextInput(window); return false; }
            if (ev.key.key == SDLK_BACKSPACE)
            {
                int l2 = static_cast<int>(str.size());
                if (l2 >= 3 && (static_cast<unsigned char>(str[l2 - 1]) >= 128))
                    str.resize(l2 - 3);
                else if (l2 >= 1)
                    str.resize(l2 - 1);
            }
        }
        SDL_Delay(16);
    }
    SDL_StopTextInput(window);
    return false;
}