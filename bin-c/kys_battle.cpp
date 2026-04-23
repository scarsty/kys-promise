// kys_battle.cpp - 战斗系统实现
// 对应 kys_battle.pas implementation

#include "kys_battle.h"
#include "kys_engine.h"
#include "kys_event.h"
#include "kys_main.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <string>
#include <vector>

constexpr int BROLE_COUNT = 42;
constexpr int MATE_COUNT = 12;
constexpr int ENEMY_COUNT = 30;

static FILE* PasOpenB(const std::string& path, const char* mode)
{
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), mode);
    return f;
}

// ==== Battle - 战斗主入口 ====
bool Battle(int battlenum, int getexp)
{
    for (int i = 0; i < 42; i++)
        BRole[i].Show = 1;
    isbattle = true;
    Bstatus = 0;
    int b = 0;
    int W = Where;
    CurrentBattle = battlenum;
    if (InitialBField())
    {
        int SelectTeamList = SelectTeamMembers();
        BRoleAmount = 0;
        for (int i = 0; i < 12; i++)
        {
            if (SelectTeamList & (1 << (i + 1)))
            {
                int y = Warsta.mate_x[b];
                int x = Warsta.mate_y[b];
                BRole[BRoleAmount].rnum = TeamList[i];
                BRole[BRoleAmount].Team = 0;
                BRole[BRoleAmount].Y = y;
                BRole[BRoleAmount].X = x;
                BRole[BRoleAmount].Face = 2;
                if (BRole[BRoleAmount].rnum == -1)
                {
                    BRole[BRoleAmount].Dead = 1;
                    BRole[BRoleAmount].Show = 1;
                }
                else
                {
                    BRole[BRoleAmount].Dead = 0;
                    BRole[BRoleAmount].Show = 0;
                }
                BRole[BRoleAmount].Step = 0;
                BRole[BRoleAmount].Acted = 0;
                BRole[BRoleAmount].ExpGot = 0;
                BRole[BRoleAmount].Auto = -1;
                BRole[BRoleAmount].Progress = 0;
                BRole[BRoleAmount].Round = 0;
                BRole[BRoleAmount].wait = 0;
                BRole[BRoleAmount].frozen = 0;
                BRole[BRoleAmount].killed = 0;
                BRole[BRoleAmount].Knowledge = 0;
                BRole[BRoleAmount].AddAtt = 0;
                BRole[BRoleAmount].AddDef = 0;
                BRole[BRoleAmount].AddSpd = 0;
                BRole[BRoleAmount].AddDodge = 0;
                BRole[BRoleAmount].AddStep = 0;
                BRole[BRoleAmount].PerfectDodge = 0;
                b++;
                BRoleAmount++;
            }
        }
        // 非队伍自动参战角色
        for (int i = 0; i < 12; i++)
        {
            if (Warsta.mate[i] > 0 && RRole[Warsta.mate[i]].TeamState != 1)
            {
                int y = Warsta.mate_x[b];
                int x = Warsta.mate_y[b];
                BRole[BRoleAmount].rnum = Warsta.mate[i];
                BRole[BRoleAmount].Team = 0;
                BRole[BRoleAmount].Y = y;
                BRole[BRoleAmount].X = x;
                BRole[BRoleAmount].Face = 2;
                BRole[BRoleAmount].Dead = (BRole[BRoleAmount].rnum == -1) ? 1 : 0;
                BRole[BRoleAmount].Show = (BRole[BRoleAmount].rnum == -1) ? 1 : 0;
                BRole[BRoleAmount].Step = 0;
                BRole[BRoleAmount].Acted = 0;
                BRole[BRoleAmount].ExpGot = 0;
                BRole[BRoleAmount].Auto = -1;
                BRole[BRoleAmount].Progress = 0;
                BRole[BRoleAmount].Round = 0;
                BRole[BRoleAmount].wait = 0;
                BRole[BRoleAmount].frozen = 0;
                BRole[BRoleAmount].killed = 0;
                BRole[BRoleAmount].Knowledge = 0;
                BRole[BRoleAmount].AddAtt = 0;
                BRole[BRoleAmount].AddDef = 0;
                BRole[BRoleAmount].AddSpd = 0;
                BRole[BRoleAmount].AddDodge = 0;
                BRole[BRoleAmount].AddStep = 0;
                BRole[BRoleAmount].PerfectDodge = 0;
                b++;
                BRoleAmount++;
            }
        }
    }

    PlayMP3(Warsta.battlemusic, -1);
    InitialWholeBField();
    CalMoveAbility();

    if (BattleMode == 0)
        OldBattleMainControl();
    else
        BattleMainControl();

    isbattle = false;
    bool result = (Bstatus == 1);
    if (result)
    {
        if (getexp > 0) AddExp();
        CheckLevelUp();
        CheckBook();
    }
    RestoreRoleStatus();
    PlayMP3(ExitSceneMusicNum, -1);
    return result;
}

// ==== InitialBField ====
bool InitialBField()
{
    FILE* sta = PasOpenB(AppPath + "resource/war.sta", "rb");
    if (!sta) return true;
    fseek(sta, 0, SEEK_END);
    long fsize = ftell(sta);
    if (fsize < static_cast<long>(CurrentBattle * sizeof(TWarSta)))
        CurrentBattle = 0;
    fseek(sta, CurrentBattle * sizeof(TWarSta), SEEK_SET);
    fread(&Warsta, sizeof(TWarSta), 1, sta);
    fclose(sta);

    int fieldnum = Warsta.battlemap;
    int offset = 0;
    if (fieldnum != 0)
    {
        FILE* idx = PasOpenB(AppPath + "resource/warfld.idx", "rb");
        if (idx) { fseek(idx, (fieldnum - 1) * 4, SEEK_SET); fread(&offset, 4, 1, idx); fclose(idx); }
    }
    FILE* grp = PasOpenB(AppPath + "resource/warfld.grp", "rb");
    if (grp)
    {
        fseek(grp, offset, SEEK_SET);
        fread(&BField[0][0][0], 2, 64 * 64 * 2, grp);
        fclose(grp);
    }
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            BField[2][i1][i2] = -1;
            BField[5][i1][i2] = -1;
            BField[4][i1][i2] = -1;
        }
    BRoleAmount = 0;
    bool result = true;
    for (int i = 0; i < 42; i++)
    {
        BRole[i].Team = 1;
        BRole[i].rnum = -1;
    }
    int autocount = 0;

    // 自动参战数据
    for (int i = 0; i < 12; i++)
    {
        int y = Warsta.mate_x[i];
        int x = Warsta.mate_y[i];
        BRole[BRoleAmount].rnum = Warsta.automate[i];
        BRole[BRoleAmount].Team = 0;
        BRole[BRoleAmount].Y = y;
        BRole[BRoleAmount].X = x;
        BRole[BRoleAmount].Face = 2;
        if (BRole[BRoleAmount].rnum == -1) { BRole[BRoleAmount].Dead = 1; BRole[BRoleAmount].Show = 1; }
        else { BRole[BRoleAmount].Dead = 0; BRole[BRoleAmount].Show = 0; autocount++; }
        BRole[BRoleAmount].Step = 0;
        BRole[BRoleAmount].Acted = 0;
        BRole[BRoleAmount].ExpGot = 0;
        BRole[BRoleAmount].Auto = -1;
        BRole[BRoleAmount].Progress = 0;
        BRole[BRoleAmount].Round = 0;
        BRole[BRoleAmount].wait = 0;
        BRole[BRoleAmount].frozen = 0;
        BRole[BRoleAmount].killed = 0;
        BRole[BRoleAmount].Knowledge = 0;
        BRole[BRoleAmount].AddAtt = 0;
        BRole[BRoleAmount].AddDef = 0;
        BRole[BRoleAmount].AddSpd = 0;
        BRole[BRoleAmount].AddDodge = 0;
        BRole[BRoleAmount].AddStep = 0;
        BRole[BRoleAmount].PerfectDodge = 0;
        BRoleAmount++;
    }
    if (autocount > 0) result = false;

    // 敌方
    for (int i = 0; i < 30; i++)
    {
        int y = Warsta.enemy_x[i];
        int x = Warsta.enemy_y[i];
        BRole[BRoleAmount].rnum = Warsta.enemy[i];
        BRole[BRoleAmount].Team = 1;
        BRole[BRoleAmount].Y = y;
        BRole[BRoleAmount].X = x;
        BRole[BRoleAmount].Face = 1;
        if (BRole[BRoleAmount].rnum == -1) { BRole[BRoleAmount].Dead = 1; BRole[BRoleAmount].Show = 1; }
        else { BRole[BRoleAmount].Dead = 0; BRole[BRoleAmount].Show = 0; }
        BRole[BRoleAmount].Step = 0;
        BRole[BRoleAmount].Acted = 0;
        BRole[BRoleAmount].ExpGot = 0;
        BRole[BRoleAmount].Auto = -1;
        BRole[BRoleAmount].Progress = 0;
        BRole[BRoleAmount].Round = 0;
        BRole[BRoleAmount].wait = 0;
        BRole[BRoleAmount].frozen = 0;
        BRole[BRoleAmount].killed = 0;
        BRole[BRoleAmount].Knowledge = 0;
        BRole[BRoleAmount].AddAtt = 0;
        BRole[BRoleAmount].AddDef = 0;
        BRole[BRoleAmount].AddSpd = 0;
        BRole[BRoleAmount].AddDodge = 0;
        BRole[BRoleAmount].AddStep = 0;
        BRole[BRoleAmount].PerfectDodge = 0;
        BRole[BRoleAmount].Show = 0;
        BRoleAmount++;
    }
    return result;
}

// ==== CalMoveAbility ====
void CalMoveAbility()
{
    maxspeed = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        int rnum = BRole[i].rnum;
        if (rnum > -1)
        {
            int addspeed = 0;
            if (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1],
                    RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 5)
                addspeed += 30;
            BRole[i].speed = GetRoleSpeed(BRole[i].rnum, true) + addspeed;
            if (BRole[i].wait == 0)
                BRole[i].Step = BRole[i].speed / 15 + std::min<int>(1, BRole[i].AddStep) * 3;
            if (BRole[i].AddSpd > 0)
            {
                BRole[i].speed = static_cast<int>(BRole[i].speed * 1.4);
                if (GetEquipState(BRole[i].rnum, 3) || GetGongtiState(BRole[i].rnum, 3))
                    BRole[i].speed = static_cast<int>(BRole[i].speed * 1.4);
            }
            maxspeed = std::max<int>(maxspeed, BRole[i].speed);
            if (RRole[rnum].Moveable > 0) BRole[i].Step = 0;
        }
    }
}

// ==== BattleStatus ====
int BattleStatus()
{
    int sum0 = 0, sum1 = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Team == 0 && BRole[i].rnum >= 0 && BRole[i].Dead == 0) sum0++;
        if (BRole[i].Team == 1 && BRole[i].rnum >= 0 && BRole[i].Dead == 0) sum1++;
    }
    if (sum0 > 0 && sum1 > 0) return 0;
    if (sum0 >= 0 && sum1 == 0) return 1;
    if (sum0 == 0 && sum1 > 0) return 2;
    return 0;
}

// ==== CalRNum ====
int CalRNum(int team)
{
    int count = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].rnum > -1 && BRole[i].Team == team && BRole[i].Dead == 0)
            count++;
    }
    return count;
}

// ==== SeekPath ====
void SeekPath(int x, int y, int step)
{
    if (x < 0 || x > 63 || y < 0 || y > 63) return;
    if (BField[2][x][y] >= 0) return;
    if (step < 0) return;
    if (BField[4][x][y] >= step) return;
    BField[4][x][y] = static_cast<int16_t>(step);
    SeekPath(x - 1, y, step - 1);
    SeekPath(x + 1, y, step - 1);
    SeekPath(x, y - 1, step - 1);
    SeekPath(x, y + 1, step - 1);
}

// ==== SeekPath2 ====
void SeekPath2(int x, int y, int step, int myteam, int mode)
{
    // BFS 寻路
    struct Pos { int x, y, s; };
    std::vector<Pos> queue_;
    queue_.push_back({x, y, step});
    BField[4][x][y] = static_cast<int16_t>(step);
    int front = 0;
    while (front < static_cast<int>(queue_.size()))
    {
        auto [cx, cy, cs] = queue_[front++];
        if (cs <= 0) continue;
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx > 63 || ny < 0 || ny > 63) continue;
            if (BField[4][nx][ny] >= cs - 1) continue;
            if (mode == 0 && BField[2][nx][ny] >= 0) continue;
            BField[4][nx][ny] = static_cast<int16_t>(cs - 1);
            queue_.push_back({nx, ny, cs - 1});
        }
    }
}

// ==== CalCanSelect ====
void CalCanSelect(int bnum, int mode, int step)
{
    for (int x = 0; x < 64; x++)
        for (int y = 0; y < 64; y++)
            BField[4][x][y] = -1;
    SeekPath2(BRole[bnum].X, BRole[bnum].Y, step, BRole[bnum].Team, mode);
}

// ==== RestoreRoleStatus ====
void RestoreRoleStatus()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        int rnum = BRole[i].rnum;
        if (rnum >= 0 && BRole[i].Team == 0)
        {
            int hp = RRole[rnum].CurrentHP;
            int mp = RRole[rnum].CurrentMP;
            if (hp <= 0) hp = 1;
            RRole[rnum].CurrentHP = static_cast<int16_t>(hp);
            RRole[rnum].CurrentMP = static_cast<int16_t>(mp);
            RRole[rnum].PhyPower = MAX_PHYSICAL_POWER;
        }
    }
}

// ==== AddExp ====
void AddExp()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        int rnum = BRole[i].rnum;
        if (rnum >= 0 && BRole[i].Team == 0)
        {
            RRole[rnum].Exp += static_cast<int16_t>(BRole[i].ExpGot);
            RRole[rnum].ExpForBook += static_cast<int16_t>(BRole[i].ExpGot);
        }
    }
}

// ==== CheckLevelUp ====
void CheckLevelUp()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Team == 0)
        {
            int rnum = BRole[i].rnum;
            if (rnum >= 0)
            {
                while (RRole[rnum].Level < MAX_LEVEL &&
                    static_cast<uint16_t>(RRole[rnum].Exp) >= static_cast<uint16_t>(LevelUpList[RRole[rnum].Level - 1]))
                {
                    RRole[rnum].Exp -= LevelUpList[RRole[rnum].Level - 1];
                    RRole[rnum].Level++;
                    LevelUp(i);
                }
            }
        }
    }
}

// ==== LevelUp ====
void LevelUp(int bnum)
{
    int rnum = BRole[bnum].rnum;
    if (rnum < 0) return;
    RRole[rnum].MaxHP += (150 - RRole[rnum].Aptitude) / 10 + rand() % 3 + 1;
    if (RRole[rnum].MaxHP > MAX_HP) RRole[rnum].MaxHP = MAX_HP;
    RRole[rnum].CurrentHP = RRole[rnum].MaxHP;

    RRole[rnum].MaxMP += (150 - RRole[rnum].Aptitude) / 10 + rand() % 3 + 1;
    if (RRole[rnum].MaxMP > MAX_MP) RRole[rnum].MaxMP = MAX_MP;
    RRole[rnum].CurrentMP = RRole[rnum].MaxMP;

    RRole[rnum].Attack += RRole[rnum].Level % 2;
    RRole[rnum].Speed += RRole[rnum].Level % 2;
    RRole[rnum].Defence += RRole[rnum].Level % 2;

    if (GetRoleMedcine(rnum, false) >= 20) RRole[rnum].Medcine++;
    if (GetRoleUsePoi(rnum, false) >= 20)  RRole[rnum].UsePoi++;
    if (GetRoleMedPoi(rnum, false) >= 20)  RRole[rnum].MedPoi++;
    if (GetRoleFist(rnum, false) >= 20)    RRole[rnum].Fist++;
    if (GetRoleSword(rnum, false) >= 20)   RRole[rnum].Sword++;
    if (GetRoleKnife(rnum, false) >= 20)   RRole[rnum].Knife++;
    if (GetRoleUnusual(rnum, false) >= 20) RRole[rnum].Unusual++;
    if (GetRoleHidWeapon(rnum, false) >= 20) RRole[rnum].HidWeapon++;

    RRole[rnum].PhyPower = MAX_PHYSICAL_POWER;
    RRole[rnum].Hurt = 0;
    RRole[rnum].Poision = 0;
    for (int i = 43; i <= 54; i++)
        RRole[rnum].Data[i] = std::min<int16_t>(RRole[rnum].Data[i], MaxProList[i]);
    if (BRole[bnum].Team == 0)
    {
        Redraw();
        NewShowStatus(rnum);
    }
}

// ==== CheckBook ====
void CheckBook()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        int rnum = BRole[i].rnum;
        if (rnum < 0) continue;
        int inum = RRole[rnum].PracticeBook;
        if (inum < 0) continue;
        int mlevel = 1;
        int mnum = RItem[inum].Magic;
        if (mnum > 0)
        {
            for (int i1 = 0; i1 < 10; i1++)
                if (RRole[rnum].Magic[i1] == mnum)
                {
                    mlevel = RRole[rnum].MagLevel[i1] / 100 + 1;
                    break;
                }
        }
        int Aptitude;
        if (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1],
                RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 2)
            Aptitude = 100;
        else
            Aptitude = RRole[rnum].Aptitude;
        int needexp;
        if (RItem[inum].NeedExp > 0)
            needexp = mlevel * (RItem[inum].NeedExp * (8 - Aptitude / 15)) / 2;
        else
            needexp = mlevel * ((-RItem[inum].NeedExp) * (1 + Aptitude / 15)) / 2;

        while (RRole[rnum].PracticeBook >= 0 && RRole[rnum].ExpForBook >= needexp && mlevel < 10)
        {
            if (RMagic[mnum].MagicType == 5 && mlevel > 1) break;
            Redraw();
            EatOneItem(rnum, inum);
            if (mnum > 0)
            {
                instruct_33(rnum, mnum, 1);
                mlevel = GetMagicLevel(rnum, mnum) / 100 + 1;
                Redraw();
            }
            RRole[rnum].ExpForBook -= static_cast<int16_t>(needexp);
            if (RItem[inum].NeedExp > 0)
                needexp = mlevel * (RItem[inum].NeedExp * (8 - Aptitude / 15)) / 2;
            else
                needexp = mlevel * ((-RItem[inum].NeedExp) * (1 + Aptitude / 15)) / 2;
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            if (GetMagicLevel(rnum, mnum) >= 900 || RMagic[mnum].MagicType == 5)
            {
                RRole[rnum].PracticeBook = -1;
                instruct_32(inum, 1);
            }
        }
    }
}

// ==== CalNewHurtValue ====
int CalNewHurtValue(int lv, int min_, int max_, int Proportion)
{
    double r = static_cast<double>(lv) / 10.0;
    return static_cast<int>(min_ + (max_ - min_) * std::pow(r, static_cast<double>(Proportion) / 100.0));
}

// ==== ShowHurtValue (mode overload) ====
void ShowHurtValue(int mode)
{
    int sign = -1;
    int color1 = 0, color2 = 0;
    switch (mode)
    {
    case 0: color1 = ColColor(0x10); color2 = ColColor(0x14); sign = -1; break;
    case 1: color1 = ColColor(0x50); color2 = ColColor(0x53); sign = -1; break;
    case 2: color1 = ColColor(0x30); color2 = ColColor(0x32); sign = -1; break;
    case 3: color1 = ColColor(0x5);  color2 = ColColor(0x7);  sign = 1;  break;
    case 4: color1 = ColColor(0x91); color2 = ColColor(0x93); sign = -1; break;
    }
    ShowHurtValue(sign, color1, color2);
}

// ==== ShowHurtValue (sign overload) ====
void ShowHurtValue(int sign, int color1, int color2)
{
    std::string fmt = (sign < 0) ? "-{}" : "+{}";
    std::vector<std::wstring> words(BROLE_COUNT);
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].ShowNumber >= 0 && BRole[i].Dead == 0 && BRole[i].rnum >= 0)
        {
            if (BRole[i].ShowNumber == 0) words[i] = L"Miss";
            else
            {
                std::string s = (sign < 0)
                    ? std::format("-{}", BRole[i].ShowNumber)
                    : std::format("+{}", BRole[i].ShowNumber);
                words[i] = std::wstring(s.begin(), s.end());
            }
        }
        else words[i] = L"0";
    }
    for (int i1 = 0; i1 <= 10; i1++)
    {
        Redraw();
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].ShowNumber >= 0 && BRole[i].Dead == 0 && BRole[i].rnum >= 0)
            {
                int x = -(BRole[i].X - Bx) * 18 + (BRole[i].Y - By) * 18 + CENTER_X - 10;
                int y = (BRole[i].X - Bx) * 9 + (BRole[i].Y - By) * 9 + CENTER_Y - 60;
                DrawEngShadowText(reinterpret_cast<uint16_t*>(words[i].data()),
                    x, y - i1 * 2, color1, color2);
            }
        }
        SDL_Delay((20 * GameSpeed) / 10);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
    Redraw();
    for (int i = 0; i < BROLE_COUNT; i++)
        BRole[i].ShowNumber = -1;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== ShowHurtValue (wstring overload) ====
void ShowHurtValue(const std::wstring& str, int color1, int color2)
{
    for (int i1 = 0; i1 <= 10; i1++)
    {
        Redraw();
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].ShowNumber >= 0 && BRole[i].Dead == 0 && BRole[i].rnum >= 0)
            {
                int x = -(BRole[i].X - Bx) * 18 + (BRole[i].Y - By) * 18 + CENTER_X - 10;
                int y = (BRole[i].X - Bx) * 9 + (BRole[i].Y - By) * 9 + CENTER_Y - 60;
                DrawShadowText(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(str.c_str())),
                    x - 20, y - i1 * 2, color1, color2);
            }
        }
        SDL_Delay((25 * GameSpeed) / 10);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
    Redraw();
    for (int i = 0; i < BROLE_COUNT; i++)
        BRole[i].ShowNumber = -1;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== ReMoveHurt ====
int16_t ReMoveHurt(int bnum, int AttackBnum)
{
    std::wstring str = L" 轉移";
    int result = bnum;
    std::vector<int> temp;
    if (rand() % 100 < 30 + RRole[BRole[bnum].rnum].Aptitude / 5)
    {
        for (int i = 0; i < BROLE_COUNT; i++)
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && i != bnum && i != AttackBnum &&
                BRole[i].Team != BRole[bnum].Team &&
                (abs(BRole[i].X - BRole[bnum].X) + abs(BRole[i].Y - BRole[bnum].Y)) <= 5)
                temp.push_back(i);
        if (!temp.empty())
        {
            int idx = temp[rand() % temp.size()];
            result = idx;
            int x = -(BRole[bnum].X - Bx) * 18 + (BRole[bnum].Y - By) * 18 + CENTER_X - 10;
            int y = (BRole[bnum].X - Bx) * 9 + (BRole[bnum].Y - By) * 9 + CENTER_Y - 60;
            for (int i1 = 0; i1 <= 10; i1++)
            {
                Redraw();
                DrawShadowText(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(str.c_str())),
                    x - 30, y - i1 * 2, ColColor(0, 0x10), ColColor(0, 0x14));
                SDL_Delay((20 * GameSpeed) / 10);
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
        }
    }
    return static_cast<int16_t>(result);
}

// ==== RetortHurt ====
int16_t RetortHurt(int bnum, int AttackBnum)
{
    std::wstring str = L" 反噬";
    int result = bnum;
    if (rand() % 100 < 30 + RRole[BRole[bnum].rnum].Aptitude / 5)
    {
        result = AttackBnum;
        int x = -(BRole[bnum].X - Bx) * 18 + (BRole[bnum].Y - By) * 18 + CENTER_X - 10;
        int y = (BRole[bnum].X - Bx) * 9 + (BRole[bnum].Y - By) * 9 + CENTER_Y - 60;
        for (int i1 = 0; i1 <= 10; i1++)
        {
            Redraw();
            DrawShadowText(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(str.c_str())),
                x - 30, y - i1 * 2, ColColor(0, 0x10), ColColor(0, 0x14));
            SDL_Delay((20 * GameSpeed) / 10);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
    }
    return static_cast<int16_t>(result);
}

// ==== CalPoiHurtLife ====
void CalPoiHurtLife(int bnum)
{
    for (int i = 0; i < BROLE_COUNT; i++)
        BRole[i].ShowNumber = -1;
    if (RRole[BRole[bnum].rnum].Poision > 0 && BRole[bnum].Dead == 0)
    {
        int hurt = RRole[BRole[bnum].rnum].CurrentHP * RRole[BRole[bnum].rnum].Poision / 200;
        RRole[BRole[bnum].rnum].CurrentHP -= static_cast<int16_t>(hurt);
        if (RRole[BRole[bnum].rnum].CurrentHP < 1) RRole[BRole[bnum].rnum].CurrentHP = 1;
        if (hurt > 0) { BRole[bnum].ShowNumber = static_cast<int16_t>(hurt); ShowHurtValue(2); }
    }
    auto showStatusEffect = [&](const wchar_t* text, int col1h, int col1l, int col2h, int col2l) {
        std::wstring ws = text;
        ShowHurtValue(ws, ColColor(0, col1h), ColColor(0, col2h));
    };
    if (RRole[BRole[bnum].rnum].Hurt > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = RRole[BRole[bnum].rnum].Hurt;
        std::wstring ws = L"內傷";
        ShowHurtValue(ws, ColColor(0, 0x10), ColColor(0, 0x14));
    }
    if (BRole[bnum].frozen > 100 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].frozen;
        std::wstring ws = L"封穴";
        ShowHurtValue(ws, ColColor(0, 0x64), ColColor(0, 0x66));
    }
    if (BRole[bnum].AddAtt > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].AddAtt;
        std::wstring ws = L"金剛";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
    if (BRole[bnum].AddSpd > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].AddSpd;
        std::wstring ws = L"飛仙";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
    if (BRole[bnum].AddDef > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].AddDef;
        std::wstring ws = L"忘憂";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
    if (BRole[bnum].AddStep > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].AddStep;
        std::wstring ws = L"神行";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
    if (BRole[bnum].PerfectDodge > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].PerfectDodge;
        std::wstring ws = L"迷蹤";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
    else if (BRole[bnum].AddDodge > 0 && BRole[bnum].Dead == 0)
    {
        BRole[bnum].ShowNumber = BRole[bnum].AddDodge;
        std::wstring ws = L"閃身";
        ShowHurtValue(ws, ColColor(0, 0x5), ColColor(0, 0x7));
    }
}

// ==== ClearDeadRolePic ====
void ClearDeadRolePic()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].rnum >= 0 && RRole[BRole[i].rnum].CurrentHP <= 0)
        {
            BRole[i].Dead = 1;
            BRole[i].Show = 1;
            BField[5][BRole[i].X][BRole[i].Y] = i;
            BField[2][BRole[i].X][BRole[i].Y] = -1;
        }
    }
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead == 0 && BRole[i].rnum >= 0)
        {
            BField[2][BRole[i].X][BRole[i].Y] = i;
            BField[5][BRole[i].X][BRole[i].Y] = -1;
        }
    }
}

// ==== Wait ====
void Wait(int bnum)
{
    if (BattleMode > 0)
    {
        BRole[bnum].wait = 1;
    }
    else
    {
        TBattleRole temp = BRole[bnum];
        for (int i = bnum; i < BROLE_COUNT - 1; i++)
            BRole[i] = BRole[i + 1];
        BRole[BROLE_COUNT - 1] = temp;
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0)
                BField[2][BRole[i].X][BRole[i].Y] = i;
            else
                BField[2][BRole[i].X][BRole[i].Y] = -1;
        }
    }
}

// ==== collect ====
void collect(int bnum)
{
    BRole[bnum].Acted = 1;
    BRole[bnum].Progress += 1;
    if (BRole[bnum].Progress >= 1500) BRole[bnum].Progress = 1200;
}

// ==== Rest ====
void Rest(int bnum)
{
    BRole[bnum].Acted = 1;
    int rnum = BRole[bnum].rnum;
    if (rnum < 0) return;
    int hurtFactor = 100 - RRole[rnum].Hurt;
    RRole[rnum].CurrentHP += static_cast<int16_t>((hurtFactor * RRole[rnum].MaxHP) / 2000);
    if (RRole[rnum].CurrentHP > RRole[rnum].MaxHP) RRole[rnum].CurrentHP = RRole[rnum].MaxHP;
    RRole[rnum].CurrentMP += static_cast<int16_t>((hurtFactor * RRole[rnum].MaxMP) / 2000);
    if (RRole[rnum].CurrentMP > RRole[rnum].MaxMP) RRole[rnum].CurrentMP = RRole[rnum].MaxMP;
    RRole[rnum].PhyPower += static_cast<int16_t>((hurtFactor * MAX_PHYSICAL_POWER) / 2000);
    if (RRole[rnum].PhyPower > MAX_PHYSICAL_POWER) RRole[rnum].PhyPower = static_cast<int16_t>(MAX_PHYSICAL_POWER);
    BRole[bnum].Progress -= 240;
    int spd = std::max<int>(1, BRole[bnum].speed / 15);
    BRole[bnum].Progress += static_cast<int16_t>((BRole[bnum].Step * 120) / spd);
}

// ==== ShowSimpleStatus ====
void ShowSimpleStatus(int rnum, int x, int y)
{
    y -= 20;
    DrawRectangle(x, y, 300, 115, 0, ColColor(255), 30);
    drawPngPic(battlepic, x, y, 0);
    int bnum = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
        if (BRole[i].rnum == rnum) { bnum = i; break; }
    // Status effects display
    std::vector<int> eft;
    if (RRole[rnum].Poision > 0) eft.push_back(0);
    if (RRole[rnum].Hurt > 0) eft.push_back(1);
    if (BRole[bnum].frozen > 0) eft.push_back(2);
    int c = (int)eft.size();
    if (c > 0)
    {
        uint32_t nt = SDL_GetTicks();
        int nt2 = (int)(nt / 10);
        int nt_val = nt2 % (200 * c);
        int nt2_val = (nt_val / 100) % 2;
        int idx_ = nt_val / 200;
        if (idx_ < c)
        {
            int fval = 100 * nt2_val + (int)pow(-1.0, nt2_val) * (nt_val % 100);
            switch (eft[idx_])
            {
                case 0: green = (RRole[rnum].Poision * fval) / 100; break;
                case 1: red = (RRole[rnum].Hurt * fval) / 100; break;
                case 2: gray = (BRole[bnum].frozen * fval) / 500; break;
            }
        }
    }
    DrawHeadPic(RRole[rnum].HeadNum, x + 22, y + 64);
    std::wstring namestr = GBKToUnicode((const char*)RRole[rnum].Name);
    UpdateHpMp(rnum, x + 77, y + 5);
    green = 0; red = 0; gray = 0;
    int namelen = (int)strlen((const char*)RRole[rnum].Name);
    std::wstring lvlabel = L" 等級";
    DrawShadowText(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(namestr.c_str())),
        x + 30 - namelen * 5, y + 69, ColColor(0x63), ColColor(0x66));
    DrawShadowText(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(lvlabel.c_str())),
        x + 77, y + 5, ColColor(0x21), ColColor(0x23));
    if (BattleMode > 0)
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].rnum == rnum)
            {
                for (int n = 0; n < (BRole[i].Progress + 1) / 300; n++)
                    drawPngPic(NowPROGRESS_PIC, n * 25 + x + 170, y + 5, 0);
            }
        }
        if (BattleMode == 2)
        {
            drawPngPic(angryprogress_pic, x, y, 0);
            if (RRole[rnum].Angry < 100)
                drawPngPic(angrycollect_pic, 0, 0, 27 + RRole[rnum].Angry, angrycollect_pic.pic->h, x, y, 0);
            else
                drawPngPic(angryfull_pic, x, y, 0);
        }
    }
    char lvbuf[32];
    snprintf(lvbuf, sizeof(lvbuf), "%d", RRole[rnum].Level);
    std::wstring lvstr(lvbuf, lvbuf + strlen(lvbuf));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(lvstr.data()),
        x + 143, y + 5, ColColor(0x5), ColColor(0x7));
}

// ==== showprogress ====
void showprogress()
{
    if (BattleMode == 0) return;
    int x = 250, y = 30;
    int b[BROLE_COUNT];
    for (int i = 0; i < BROLE_COUNT; i++) b[i] = i;
    // sort by progress ascending
    for (int i1 = 0; i1 < BROLE_COUNT - 1; i1++)
        for (int i2 = i1 + 1; i2 < BROLE_COUNT; i2++)
        {
            if (BRole[b[i1]].rnum > -1 && BRole[b[i2]].rnum > -1)
            {
                if (BRole[b[i1]].Progress % 300 > BRole[b[i2]].Progress % 300)
                    std::swap(b[i1], b[i2]);
            }
        }
    drawPngPic(PROGRESS_PIC, x, y, 0);
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[b[i]].rnum >= 0 && BRole[b[i]].Dead == 0)
        {
            int px = 20 + (BRole[b[i]].Progress) % 300 + x;
            if (BRole[b[i]].Team == 0)
            {
                if (BField[4][BRole[b[i]].X][BRole[b[i]].Y] > 0)
                    drawPngPic(SELECTEDMATE_PIC, px, y, 0);
                else
                    drawPngPic(MATESIGN_PIC, px, y, 0);
                ZoomPic(Head_Pic[RRole[BRole[b[i]].rnum].HeadNum].pic, 0, px - 10, y - 30, 29, 30);
            }
            else
            {
                if (BField[4][BRole[b[i]].X][BRole[b[i]].Y] > 0)
                    drawPngPic(SELECTEDENEMY_PIC, px, y, 0);
                else
                    drawPngPic(ENEMYSIGN_PIC, px, y, 0);
                ZoomPic(Head_Pic[RRole[BRole[b[i]].rnum].HeadNum].pic, 0, px - 10, y + 30, 29, 30);
            }
        }
    }
}

// ==== SelectTeamMembers ====
int SelectTeamMembers()
{
    int result = 0, max_ = 1, menu = 0;
    MenuString.clear();
    MenuString.resize(8);
    MenuString[0] = L"    全員出戰";
    for (int i = 0; i < 6; i++)
    {
        if (TeamList[i] >= 0)
        {
            MenuString[i + 1] = GBKToUnicode((const char*)RRole[TeamList[i]].Name);
            max_++;
        }
    }
    MenuString[max_] = L"    開始戰鬥";
    ShowMultiMenu(max_, 0, 0);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                if (menu != max_ && menu != 0)
                {
                    result ^= (1 << menu);
                    ShowMultiMenu(max_, menu, result);
                }
                else if (menu == 0)
                {
                    for (int i = 0; i < 6; i++)
                        if (TeamList[i] >= 0) result ^= (1 << (i + 1));
                    ShowMultiMenu(max_, menu, result);
                }
                else if (menu == max_ && result != 0) goto done_select;
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8)
            {
                menu--; if (menu < 0) menu = max_;
                ShowMultiMenu(max_, menu, result);
            }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2)
            {
                menu++; if (menu > max_) menu = 0;
                ShowMultiMenu(max_, menu, result);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (menu != max_ && menu != 0) { result ^= (1 << menu); ShowMultiMenu(max_, menu, result); }
                else if (menu == max_ && result != 0) goto done_select;
                else if (menu == 0) { for (int i = 0; i < 6; i++) if (TeamList[i] >= 0) result ^= (1 << (i + 1)); ShowMultiMenu(max_, menu, result); }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= CENTER_X - 75 && xm < CENTER_X + 75 && ym >= 100 && ym < max_ * 22 + 128)
            {
                int mp = menu;
                menu = (ym - 102) / 22;
                if (mp != menu) ShowMultiMenu(max_, menu, result);
            }
        }
        break;
        }
    }
done_select:
    return result;
}

// ==== ShowMultiMenu ====
void ShowMultiMenu(int max_, int menu, int status)
{
    int x = CENTER_X - 105, y = 100;
    Redraw();
    std::wstring str1 = L" 參戰";
    DrawRectangle(x + 30, y, 150, max_ * 22 + 28, 0, ColColor(0, 255), 30);
    for (int i = 0; i <= max_; i++)
    {
        uint16_t* ms = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(MenuString[i].c_str()));
        uint16_t* s1 = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(str1.c_str()));
        if (i == menu)
        {
            DrawShadowText(ms, x + 13, y + 3 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
            if ((status & (1 << i)) > 0)
                DrawShadowText(s1, x + 113, y + 3 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
        }
        else
        {
            DrawShadowText(ms, x + 13, y + 3 + 22 * i, ColColor(0, 0x5), ColColor(0, 0x7));
            if ((status & (1 << i)) > 0)
                DrawShadowText(s1, x + 113, y + 3 + 22 * i, ColColor(0, 0x21), ColColor(0, 0x23));
        }
    }
    SDL_UpdateRect2(screen, x + 30, y, 151, max_ * 22 + 29);
}

// ==== CountProgress ====
int CountProgress()
{
    double b = 1.0;
    int result = -1;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].rnum >= 0 && BRole[i].Dead == 0 && BRole[i].wait == 0)
        {
            if (BRole[i].Progress % 300 + (int)(BRole[i].speed / 15.0) >= 299)
            {
                double a = (300.0 - (BRole[i].Progress % 300)) / 15.0;
                b = std::min(a, b);
                result = i;
                break;
            }
        }
    }
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].rnum >= 0 && BRole[i].Dead == 0 && BRole[i].frozen > 0)
        {
            BRole[i].frozen -= static_cast<int16_t>((int)(b * (BRole[i].speed / 15.0)) / 3);
        }
        else if (BRole[i].rnum >= 0 && BRole[i].Dead == 0 && BRole[i].wait == 0)
        {
            BRole[i].frozen = 0;
            int n = BRole[i].Progress / 300;
            BRole[i].Progress += static_cast<int16_t>((int)(b * (BRole[i].speed / 15.0)));
            if (BRole[i].Progress / 300 > n) BRole[i].Progress = static_cast<int16_t>(n * 300 + 299);
            if (i == result) BRole[i].Progress = static_cast<int16_t>(n * 300 + 299);
        }
    }
    showprogress();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    return result;
}

// ==== ReArrangeBRole ====
void ReArrangeBRole()
{
    for (int i1 = 0; i1 < BROLE_COUNT - 1; i1++)
        for (int i2 = i1 + 1; i2 < BROLE_COUNT; i2++)
        {
            int s1 = 0, s2 = 0;
            if (BRole[i1].rnum > -1 && BRole[i1].Dead == 0)
            {
                s1 = GetRoleSpeed(BRole[i1].rnum, true);
                if (CheckEquipSet(RRole[BRole[i1].rnum].Equip[0], RRole[BRole[i1].rnum].Equip[1],
                    RRole[BRole[i1].rnum].Equip[2], RRole[BRole[i1].rnum].Equip[3]) == 5)
                    s1 += 30;
            }
            if (BRole[i2].rnum > -1 && BRole[i2].Dead == 0)
            {
                s2 = GetRoleSpeed(BRole[i2].rnum, true);
                if (CheckEquipSet(RRole[BRole[i2].rnum].Equip[0], RRole[BRole[i2].rnum].Equip[1],
                    RRole[BRole[i2].rnum].Equip[2], RRole[BRole[i2].rnum].Equip[3]) == 5)
                    s2 += 30;
            }
            bool pet51 = (GetPetSkill(5, 1) && BRole[i1].rnum == 0) || (GetPetSkill(5, 3) && BRole[i1].Team == 0);
            bool pet52 = (GetPetSkill(5, 1) && BRole[i2].rnum == 0) || (GetPetSkill(5, 3) && BRole[i2].Team == 0);
            if (!pet51 && (s1 < s2 || pet52))
                std::swap(BRole[i1], BRole[i2]);
        }
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        { BField[2][i1][i2] = -1; BField[5][i1][i2] = -1; }
    int n = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
        if (BRole[i].Dead == 0 && BRole[i].rnum >= 0) n++;
    int n1 = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].rnum >= 0)
        {
            if (BRole[i].Dead == 0)
            {
                BField[2][BRole[i].X][BRole[i].Y] = i;
                BField[5][BRole[i].X][BRole[i].Y] = -1;
                if (BattleMode > 0) BRole[i].Progress = static_cast<int16_t>((n - n1) * 5);
                n1++;
            }
            else
            {
                BField[2][BRole[i].X][BRole[i].Y] = -1;
                BField[5][BRole[i].X][BRole[i].Y] = i;
            }
        }
    }
    if (BattleMode > 0)
    {
        int idx2 = 0;
        for (int i1 = 0; i1 < BROLE_COUNT; i1++)
            if ((GetPetSkill(5, 1) && BRole[i1].rnum == 0) || (GetPetSkill(5, 3) && BRole[i1].Team == 0))
            { BRole[i1].Progress = static_cast<int16_t>(299 - idx2 * 5); idx2++; }
    }
}

// ==== BattleMainControl ====
void BattleMainControl()
{
    CalMoveAbility();
    ReArrangeBRole();
    Bx = BRole[0].X; By = BRole[0].Y;
    for (int i = 0; i < BROLE_COUNT; i++) BRole[i].Round = 0;
    CallEvent(Warsta.BoutEvent);
    for (int i = 0; i < BROLE_COUNT; i++) { BRole[i].Round = 1; BRole[i].LifeAdd = 0; }

    while (Bstatus == 0)
    {
        Redraw();
        int i = CountProgress();
        if (i >= 0)
        {
            CurBRole = i;
            BRole[i].Acted = 0;
            CallEvent(Warsta.BoutEvent);
            Bx = BRole[i].X; By = BRole[i].Y;
            if (Bstatus > 0) break;
            Redraw(); showprogress();
            for (int i1 = 0; i1 < 64; i1++)
                for (int i2 = 0; i2 < 64; i2++)
                    BField[4][i1][i2] = 0;
            // check for ESC
            SDL_PollEvent(&event);
            CheckBasicEvent();
            if (event.key.key == SDLK_ESCAPE || event.button.button == SDL_BUTTON_RIGHT)
            { BRole[i].Auto = -1; event.button.button = 0; event.key.key = 0; }
            x50[28005] = static_cast<int16_t>(i);
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].Acted == 0)
            {
                CurBRole = i;
                if (BRole[i].LifeAdd == 0)
                {
                    BRole[i].AddAtt = std::max<int16_t>(0, BRole[i].AddAtt - 1);
                    BRole[i].AddDef = std::max<int16_t>(0, BRole[i].AddDef - 1);
                    BRole[i].AddSpd = std::max<int16_t>(0, BRole[i].AddSpd - 1);
                    BRole[i].AddDodge = std::max<int16_t>(0, BRole[i].AddDodge - 1);
                    BRole[i].AddStep = std::max<int16_t>(0, BRole[i].AddStep - 1);
                    BRole[i].PerfectDodge = std::max<int16_t>(0, BRole[i].PerfectDodge - 1);
                    if (GetEquipState(BRole[i].rnum, 11) || GetGongtiState(BRole[i].rnum, 11))
                    {
                        int add = RRole[BRole[i].rnum].MaxHP / 10;
                        if (RRole[BRole[i].rnum].MaxHP < RRole[BRole[i].rnum].CurrentHP + add)
                            add = RRole[BRole[i].rnum].MaxHP - RRole[BRole[i].rnum].CurrentHP;
                        for (int a = 0; a < BROLE_COUNT; a++) BRole[a].ShowNumber = -1;
                        BRole[i].ShowNumber = static_cast<int16_t>(add);
                        if (add > 0) ShowHurtValue(3);
                        RRole[BRole[i].rnum].CurrentHP += static_cast<int16_t>(add);
                    }
                    if (GetEquipState(BRole[i].rnum, 23) || GetGongtiState(BRole[i].rnum, 23))
                    {
                        int add = RRole[BRole[i].rnum].MaxMP / 20;
                        if (RRole[BRole[i].rnum].MaxMP < RRole[BRole[i].rnum].CurrentMP + add)
                            add = RRole[BRole[i].rnum].MaxMP - RRole[BRole[i].rnum].CurrentMP;
                        for (int a = 0; a < BROLE_COUNT; a++) BRole[a].ShowNumber = -1;
                        BRole[i].ShowNumber = static_cast<int16_t>(add);
                        if (add > 0) ShowHurtValue(1, ColColor(0, 0x50), ColColor(0, 0x53));
                        RRole[BRole[i].rnum].CurrentMP += static_cast<int16_t>(add);
                    }
                    CalPoiHurtLife(i);
                    BRole[i].LifeAdd = 1;
                }
                if (BRole[i].Team == 0 && BRole[i].Auto == -1 && BRole[i].wait == 0)
                {
                    while (BRole[i].Acted == 0 && BRole[i].Auto == -1 && BRole[i].wait == 0)
                    {
                        for (int i1 = 0; i1 < 64; i1++)
                            for (int i2 = 0; i2 < 64; i2++)
                                BField[4][i1][i2] = 0;
                        switch (BattleMenu(i))
                        {
                        case 0: MoveRole(i); break;
                        case 1: Attack(i); break;
                        case 2: UsePoision(i); break;
                        case 3: MedPoision(i); break;
                        case 4: Medcine(i); break;
                        case 5: MedFrozen(i); break;
                        case 6: collect(i); break;
                        case 7: BattleMenuItem(i); break;
                        case 8: Wait(i); break;
                        case 9: Selectshowstatus(i); break;
                        case 10: Rest(i); break;
                        case 11: Auto(i); break;
                        }
                    }
                }
                else
                {
                    AutoBattle2(i);
                    BRole[i].Acted = 1;
                }
            }
            else if (BRole[i].Acted == 1)
            {
                BRole[i].Progress -= 300;
                showprogress();
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                SDL_Delay(500);
            }
            else
                BRole[i].Acted = 1;
            ClearDeadRolePic();
            Bstatus = BattleStatus();
            if (BRole[i].Acted == 1)
            {
                CallEvent(Warsta.OperationEvent);
                BRole[i].Round++;
                BRole[i].LifeAdd = 0;
            }
            for (int i1 = 0; i1 < 64; i1++)
                for (int i2 = 0; i2 < 64; i2++)
                    BField[4][i1][i2] = 0;
            Redraw(); showprogress();
            x50[28101] = static_cast<int16_t>(BRoleAmount);
            CalMoveAbility();
            for (int k = 0; k < BROLE_COUNT; k++)
                if (BRole[k].wait == 1 && k != i)
                { BRole[k].wait = 0; break; }
        }
        SDL_Delay((maxspeed * GameSpeed) / 1000);
    }
}

// ==== OldBattleMainControl ====
void OldBattleMainControl()
{
    for (int i = 0; i < BROLE_COUNT; i++) BRole[i].LifeAdd = 0;
    while (Bstatus == 0)
    {
        CalMoveAbility();
        ReArrangeBRole();
        ClearDeadRolePic();
        Bx = BRole[0].X; By = BRole[0].Y;
        for (int i = 0; i < BROLE_COUNT; i++) { BRole[i].Acted = 0; BRole[i].ShowNumber = 0; }
        CallEvent(Warsta.BoutEvent);
        for (int i = 0; i < BROLE_COUNT; i++) BRole[i].Round++;
        int i = 0;
        while (i < BROLE_COUNT && Bstatus == 0)
        {
            if (BRole[i].rnum < 0 || BRole[i].Dead != 0) { i++; continue; }
            CurBRole = i;
            Bx = BRole[i].X; By = BRole[i].Y;
            Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            if (BRole[i].LifeAdd == 0)
            {
                BRole[i].AddAtt = std::max<int16_t>(0, BRole[i].AddAtt - 1);
                BRole[i].AddDef = std::max<int16_t>(0, BRole[i].AddDef - 1);
                BRole[i].AddSpd = std::max<int16_t>(0, BRole[i].AddSpd - 1);
                BRole[i].AddDodge = std::max<int16_t>(0, BRole[i].AddDodge - 1);
                BRole[i].AddStep = std::max<int16_t>(0, BRole[i].AddStep - 1);
                BRole[i].PerfectDodge = std::max<int16_t>(0, BRole[i].PerfectDodge - 1);
                if (GetEquipState(BRole[i].rnum, 11) || GetGongtiState(BRole[i].rnum, 11))
                {
                    int add = RRole[BRole[i].rnum].MaxHP / 10;
                    if (RRole[BRole[i].rnum].MaxHP < RRole[BRole[i].rnum].CurrentHP + add)
                        add = RRole[BRole[i].rnum].MaxHP - RRole[BRole[i].rnum].CurrentHP;
                    for (int a = 0; a < BROLE_COUNT; a++) BRole[a].ShowNumber = -1;
                    BRole[i].ShowNumber = static_cast<int16_t>(add);
                    if (add > 0) ShowHurtValue(3);
                    RRole[BRole[i].rnum].CurrentHP += static_cast<int16_t>(add);
                }
                if (GetEquipState(BRole[i].rnum, 23) || GetGongtiState(BRole[i].rnum, 23))
                {
                    int add = RRole[BRole[i].rnum].MaxMP / 20;
                    if (RRole[BRole[i].rnum].MaxMP < RRole[BRole[i].rnum].CurrentMP + add)
                        add = RRole[BRole[i].rnum].MaxMP - RRole[BRole[i].rnum].CurrentMP;
                    for (int a = 0; a < BROLE_COUNT; a++) BRole[a].ShowNumber = -1;
                    BRole[i].ShowNumber = static_cast<int16_t>(add);
                    if (add > 0) ShowHurtValue(1, ColColor(0, 0x50), ColColor(0, 0x53));
                    RRole[BRole[i].rnum].CurrentMP += static_cast<int16_t>(add);
                }
                CalPoiHurtLife(i);
                BRole[i].LifeAdd = 1;
            }
            SDL_PollEvent(&event);
            CheckBasicEvent();
            if (event.key.key == SDLK_ESCAPE || event.button.button == SDL_BUTTON_RIGHT)
            { BRole[i].Auto = -1; event.button.button = 0; event.key.key = 0; }
            x50[28005] = static_cast<int16_t>(i);
            if (BRole[i].frozen >= 100) { BRole[i].Acted = 1; BRole[i].frozen -= 100; }
            else if (BRole[i].Dead == 0 && BRole[i].Acted == 0)
            {
                BRole[i].frozen = 0;
                if (BRole[i].Team == 0 && BRole[i].Auto == -1)
                {
                    switch (BattleMenu(i))
                    {
                    case 0: MoveRole(i); break;
                    case 1: Attack(i); break;
                    case 2: UsePoision(i); break;
                    case 3: MedPoision(i); break;
                    case 4: Medcine(i); break;
                    case 5: MedFrozen(i); break;
                    case 6: collect(i); break;
                    case 7: BattleMenuItem(i); break;
                    case 8: Wait(i); break;
                    case 9: Selectshowstatus(i); break;
                    case 10: Rest(i); break;
                    case 11: Auto(i); break;
                    }
                }
                else { AutoBattle2(i); BRole[i].Acted = 1; }
            }
            ClearDeadRolePic();
            Bstatus = BattleStatus();
            if (BRole[i].Acted == 1) { BRole[i].LifeAdd = 0; CallEvent(Warsta.OperationEvent); i++; }
            Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            SDL_Delay((200 * GameSpeed) / 10);
        }
        x50[28101] = static_cast<int16_t>(BRoleAmount);
    }
}

// ==== BattleMenu ====
int BattleMenu(int bnum)
{
    int menustatus = 0xF80, max_ = 4, rnum = BRole[bnum].rnum, menu = 0;
    if (BRole[bnum].Step > 0) { menustatus |= 1; max_++; }
    if (RRole[rnum].PhyPower >= 10)
    {
        int p = 0;
        for (int i = 0; i < 10; i++)
        {
            if (RRole[rnum].Magic[i] > 0 && RMagic[RRole[rnum].Magic[i]].NeedMP <= RRole[rnum].CurrentMP)
            {
                int lv = RRole[BRole[bnum].rnum].MagLevel[i] / 100;
                if ((BRole[bnum].Progress + 1) / 3 >= (RMagic[RRole[rnum].Magic[i]].NeedProgress * 10) * lv + 100 || BattleMode == 0 || RRole[rnum].Angry == 100)
                { p = 1; break; }
            }
        }
        if (p > 0) { menustatus |= 2; max_++; }
    }
    if (GetRoleUsePoi(rnum, true) > 0 && RRole[rnum].PhyPower >= 30) { menustatus |= 4; max_++; }
    if (GetRoleMedPoi(rnum, true) > 0 && RRole[rnum].PhyPower >= 50) { menustatus |= 8; max_++; }
    if (GetRoleMedcine(rnum, true) > 0 && RRole[rnum].PhyPower >= 50) { menustatus |= 16; max_++; }
    if ((RRole[rnum].CurrentMP + GetRoleMedcine(rnum, true) * 5 > 200) && RRole[rnum].PhyPower >= 50) { menustatus |= 32; max_++; }
    if (BattleMode > 0) { menustatus |= 64; max_++; }
    Redraw();
    DrawRectangle(10, 50, 80, 28, 0, ColColor(0, 255), 30);
    char roundbuf[32]; snprintf(roundbuf, sizeof(roundbuf), "第%d回", BRole[bnum].Round);
    std::wstring roundstr(roundbuf, roundbuf + strlen(roundbuf));
    DrawShadowText(reinterpret_cast<uint16_t*>(roundstr.data()),
        10 - 17, 52, ColColor(0, 5), ColColor(0, 7));
    ShowSimpleStatus(BRole[bnum].rnum, 30, 330);
    showprogress();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    ShowBMenu(menustatus, menu, max_);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) goto done_menu;
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu--; if (menu < 0) menu = max_; ShowBMenu(menustatus, menu, max_); }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu++; if (menu > max_) menu = 0; ShowBMenu(menustatus, menu, max_); }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                int xm, ym; SDL_GetMouseState2(xm, ym);
                if (xm >= 100 && xm < 147 && ym >= 28 && ym < max_ * 22 + 56) goto done_menu;
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 100 && xm < 147 && ym >= 28 && ym < max_ * 22 + 56)
            {
                int mp = menu; menu = (ym - 30) / 22;
                if (menu > max_) menu = max_; if (menu < 0) menu = 0;
                if (mp != menu) ShowBMenu(menustatus, menu, max_);
            }
        }
        break;
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
done_menu:
    event.key.key = 0; event.button.button = 0;
    int p = 0, ri = 0;
    for (ri = 0; ri < 12; ri++)
    {
        if ((menustatus & (1 << ri)) > 0) { p++; if (p > menu) break; }
    }
    return ri;
}

// ==== ShowBMenu ====
void ShowBMenu(int MenuStatus, int menu, int max_)
{
    std::wstring word[12] = { L" 移動", L" 武學", L" 用毒", L" 解毒", L" 醫療", L" 解穴", L" 聚氣", L" 物品", L" 等待", L" 狀態", L" 休息", L" 自動" };
    Redraw();
    DrawRectangle(100, 28, 47, max_ * 22 + 28, 0, ColColor(255), 30);
    int p = 0;
    for (int i = 0; i < 12; i++)
    {
        if ((MenuStatus & (1 << i)) > 0)
        {
            uint16_t* w = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(word[i].c_str()));
            if (p == menu) DrawShadowText(w, 83, 31 + 22 * p, ColColor(0x64), ColColor(0x66));
            else DrawShadowText(w, 83, 31 + 22 * p, ColColor(0x21), ColColor(0x23));
            p++;
        }
    }
    SDL_UpdateRect2(screen, 100, 28, 48, max_ * 22 + 29);
}

// ==== ShowBMenu2 ====
void ShowBMenu2(int MenuStatus, int menu, int max_, int bnum)
{
    std::wstring word[12] = { L" 移動", L" 武學", L" 用毒", L" 解毒", L" 醫療", L" 解穴", L" 聚氣", L" 物品", L" 等待", L" 狀態", L" 休息", L" 自動" };
    Redraw();
    ShowSimpleStatus(BRole[bnum].rnum, 30, 330);
    showprogress();
    DrawRectangle(100, 28, 47, max_ * 22 + 28, 0, ColColor(255), 30);
    int p = 0;
    for (int i = 0; i < 12; i++)
    {
        if ((MenuStatus & (1 << i)) > 0)
        {
            uint16_t* w = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(word[i].c_str()));
            if (p == menu) DrawShadowText(w, 83, 31 + 22 * p, ColColor(0x64), ColColor(0x66));
            else DrawShadowText(w, 83, 31 + 22 * p, ColColor(0x21), ColColor(0x23));
            p++;
        }
    }
    SDL_UpdateRect2(screen, 100, 28, 48, max_ * 22 + 29);
}

// ==== MoveRole ====
void MoveRole(int bnum)
{
    CalCanSelect(bnum, 0, BRole[bnum].Step);
    if (SelectAim(bnum, BRole[bnum].Step))
        MoveAmination(bnum);
}

// ==== MoveAmination ====
void MoveAmination(int bnum)
{
    if (BField[3][Ax][Ay] > 0)
    {
        int Xinc[5] = {0,1,-1,0,0}, Yinc[5] = {0,0,0,1,-1};
        linex[0] = static_cast<int16_t>(Bx); liney[0] = static_cast<int16_t>(By);
        linex[BField[3][Ax][Ay]] = static_cast<int16_t>(Ax);
        liney[BField[3][Ax][Ay]] = static_cast<int16_t>(Ay);
        int a = BField[3][Ax][Ay] - 1;
        while (a >= 0)
        {
            for (int dir = 1; dir <= 4; dir++)
            {
                int tx = linex[a + 1] + Xinc[dir], ty = liney[a + 1] + Yinc[dir];
                if (tx >= 0 && tx < 64 && ty >= 0 && ty < 64 &&
                    BField[3][tx][ty] == BField[3][linex[a + 1]][liney[a + 1]] - 1)
                { linex[a] = static_cast<int16_t>(tx); liney[a] = static_cast<int16_t>(ty); break; }
            }
            a--;
        }
        a = 1;
        while (!(BRole[bnum].Step == 0 || (Bx == Ax && By == Ay)))
        {
            SDL_PollEvent(nullptr);
            if (linex[a] - Bx > 0) BRole[bnum].Face = 3;
            else if (linex[a] - Bx < 0) BRole[bnum].Face = 0;
            else if (liney[a] - By < 0) BRole[bnum].Face = 2;
            else BRole[bnum].Face = 1;
            if (BField[2][Bx][By] == bnum) BField[2][Bx][By] = -1;
            Bx = linex[a]; By = liney[a];
            if (BField[2][Bx][By] == -1) BField[2][Bx][By] = static_cast<int16_t>(bnum);
            a++; BRole[bnum].Step--;
            Redraw(); showprogress();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            SDL_Delay((GameSpeed * 20) / 10);
        }
        BRole[bnum].X = static_cast<int16_t>(Bx); BRole[bnum].Y = static_cast<int16_t>(By);
    }
}

// ==== Selectshowstatus ====
bool Selectshowstatus(int bnum)
{
    int idx = 0;
    while (true)
    {
        while (BRole[idx].rnum < 0 || BRole[idx].Dead != 0) { idx++; if (idx >= BROLE_COUNT) idx = 0; }
        Bx = BRole[idx].X; By = BRole[idx].Y;
        Redraw();
        NewShowStatus(BRole[idx].rnum);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        while (SDL_PollEvent(&event) || true)
        {
            CheckBasicEvent();
            if (event.type == SDL_EVENT_KEY_UP)
            {
                if (event.key.key == SDLK_ESCAPE) { Bx = BRole[bnum].X; By = BRole[bnum].Y; return false; }
                if (event.key.key == SDLK_LEFT || event.key.key == SDLK_UP) { idx--; if (idx < 0) idx = BROLE_COUNT - 1; break; }
                if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_DOWN) { idx++; if (idx >= BROLE_COUNT) idx = 0; break; }
            }
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (event.button.button == SDL_BUTTON_RIGHT) { Bx = BRole[bnum].X; By = BRole[bnum].Y; return false; }
                if (event.button.button == SDL_BUTTON_LEFT) { idx++; if (idx >= BROLE_COUNT) idx = 0; break; }
            }
            event.key.key = 0; event.button.button = 0;
            SDL_Delay((20 * GameSpeed) / 10);
        }
    }
    return false;
}

// ==== SelectAim ====
bool SelectAim(int bnum, int step)
{
    Ax = Bx; Ay = By;
    DrawBFieldWithCursor(0, step, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { Ay--; if (abs(Ax-Bx)+abs(Ay-By)>step || BField[3][Ax][Ay]<0) Ay++; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { Ay++; if (abs(Ax-Bx)+abs(Ay-By)>step || BField[3][Ax][Ay]<0) Ay--; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { Ax++; if (abs(Ax-Bx)+abs(Ay-By)>step || BField[3][Ax][Ay]<0) Ax--; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { Ax--; if (abs(Ax-Bx)+abs(Ay-By)>step || BField[3][Ax][Ay]<0) Ax++; }
            DrawBFieldWithCursor(0, step, 0);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            int Axp = (-xm + CENTER_X + 2*ym - 2*CENTER_Y + 18) / 36 + Bx;
            int Ayp = (xm - CENTER_X + 2*ym - 2*CENTER_Y + 18) / 36 + By;
            if (abs(Axp-Bx)+abs(Ayp-By)<=step && Axp>=0 && Axp<64 && Ayp>=0 && Ayp<64 && BField[3][Axp][Ayp]>=0)
            {
                Ax = Axp; Ay = Ayp;
                DrawBFieldWithCursor(0, step, 0);
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
        }
        break;
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SelectRange ====
bool SelectRange(int bnum, int AttAreaType, int step, int range)
{
    Ax = Bx; Ay = By;
    DrawBFieldWithCursor(AttAreaType, step, range);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { Ay--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay++; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { Ay++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay--; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { Ax++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax--; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { Ax--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax++; }
            DrawBFieldWithCursor(AttAreaType, step, range);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            int Axp = (-xm+CENTER_X+2*ym-2*CENTER_Y+18)/36+Bx;
            int Ayp = (xm-CENTER_X+2*ym-2*CENTER_Y+18)/36+By;
            if (abs(Axp-Bx)+abs(Ayp-By)<=step) { Ax=Axp; Ay=Ayp; DrawBFieldWithCursor(AttAreaType, step, range); SDL_UpdateRect2(screen,0,0,screen->w,screen->h); }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SelectDirector ====
bool SelectDirector(int bnum, int AttAreaType, int step, int range)
{
    Ax = Bx - 1; Ay = By;
    DrawBFieldWithCursor(AttAreaType, step, range);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            { if (Ax != Bx || Ay != By) { event.key.key = 0; return true; } }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { Ay=By-1; Ax=Bx; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { Ay=By+1; Ax=Bx; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { Ax=Bx+1; Ay=By; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { Ax=Bx-1; Ay=By; }
            DrawBFieldWithCursor(AttAreaType, step, range);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (xm<CENTER_X && ym<CENTER_Y) { Ay=By-1; Ax=Bx; }
                else if (xm<CENTER_X) { Ax=Bx+1; Ay=By; }
                else if (ym<CENTER_Y) { Ax=Bx-1; Ay=By; }
                else { Ay=By+1; Ax=Bx; }
                event.button.button = 0; return true;
            }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm<CENTER_X && ym<CENTER_Y) { Ay=By-1; Ax=Bx; }
            else if (xm<CENTER_X) { Ax=Bx+1; Ay=By; }
            else if (ym<CENTER_Y) { Ax=Bx-1; Ay=By; }
            else { Ay=By+1; Ax=Bx; }
            DrawBFieldWithCursor(AttAreaType, step, range);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SelectCross ====
bool SelectCross(int bnum, int AttAreaType, int step, int range)
{
    Ax = Bx; Ay = By;
    DrawBFieldWithCursor(AttAreaType, step, range);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SelectFar ====
bool SelectFar(int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1)
        step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22))
        step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    int AttAreaType = RMagic[mnum].AttAreaType;
    int minstep = RMagic[mnum].MinStep;
    Ax = Bx - minstep - 1; Ay = By;
    DrawBFieldWithCursor(AttAreaType, step, range);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { Ay--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay++; if (abs(Ax-Bx)+abs(Ay-By)<=minstep) Ay++; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { Ay++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay--; if (abs(Ax-Bx)+abs(Ay-By)<=minstep) Ay--; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { Ax++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax--; if (abs(Ax-Bx)+abs(Ay-By)<=minstep) Ax--; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { Ax--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax++; if (abs(Ax-Bx)+abs(Ay-By)<=minstep) Ax++; }
            DrawBFieldWithCursor(AttAreaType, step, range);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            int Axp = (-xm+CENTER_X+2*ym-2*CENTER_Y+18)/36+Bx;
            int Ayp = (xm-CENTER_X+2*ym-2*CENTER_Y+18)/36+By;
            if (abs(Axp-Bx)+abs(Ayp-By)<=step && abs(Axp-Bx)+abs(Ayp-By)>minstep)
            { Ax=Axp; Ay=Ayp; DrawBFieldWithCursor(AttAreaType, step, range); SDL_UpdateRect2(screen,0,0,screen->w,screen->h); }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SelectLine ====
bool SelectLine(int bnum, int AttAreaType, int step, int range)
{
    Ax = Bx; Ay = By;
    DrawBFieldWithCursor(AttAreaType, step, range);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { Ay--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay++; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { Ay++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ay--; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { Ax++; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax--; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { Ax--; if (abs(Ax-Bx)+abs(Ay-By)>step) Ax++; }
            DrawBFieldWithCursor(AttAreaType, step, range);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            int Axp = (-xm+CENTER_X+2*ym-2*CENTER_Y+18)/36+Bx;
            int Ayp = (xm-CENTER_X+2*ym-2*CENTER_Y+18)/36+By;
            if (abs(Axp-Bx)+abs(Ayp-By)<=step)
            { Ax=Axp; Ay=Ayp; DrawBFieldWithCursor(AttAreaType, step, range); SDL_UpdateRect2(screen,0,0,screen->w,screen->h); }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}

// ==== SetAminationPosition ====
void SetAminationPosition(int mode, int step, int range)
{
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            BField[4][i1][i2] = 0;
            switch (mode)
            {
            case 0: case 6:
                if (abs(i1-Ax)+abs(i2-Ay)<=range) BField[4][i1][i2] = 1; break;
            case 3:
                if (abs(i1-Ax)<=range && abs(i2-Ay)<=range) BField[4][i1][i2] = 1; break;
            case 1:
                if ((i1==Bx || i2==By) &&
                    ((Ax-Bx==0) || ((Ax-Bx!=0)&&((i1-Bx>0)==(Ax-Bx>0)))) &&
                    abs(i1-Bx)<=step &&
                    ((Ay-By==0) || ((Ay-By!=0)&&((i2-By>0)==(Ay-By>0)))) &&
                    abs(i2-By)<=step)
                    BField[4][i1][i2] = 1; break;
            case 2:
                if ((abs(i1-Bx)==abs(i2-By) && abs(i1-Bx)<=range) ||
                    (i1==Bx && abs(i2-By)<=step) ||
                    (i2==By && abs(i1-Bx)<=step))
                    BField[4][i1][i2] = 1;
                if (i1==Bx && i2==By) BField[4][i1][i2] = 1;
                Ax = Bx; Ay = By; break;
            case 4:
                if (abs(i1-Bx)+abs(i2-By)<=step && abs(i1-Bx)!=abs(i2-By))
                {
                    if (((i1-Bx)*(Ax-Bx)>0 && abs(i1-Bx)>abs(i2-By)) ||
                        ((i2-By)*(Ay-By)>0 && abs(i1-Bx)<abs(i2-By)))
                        BField[4][i1][i2] = 1;
                } break;
            case 5:
                if (abs(i1-Bx)<=step && abs(i2-By)<=step && abs(i1-Bx)!=abs(i2-By))
                {
                    if (((i1-Bx)*(Ax-Bx)>0 && abs(i1-Bx)>abs(i2-By)) ||
                        ((i2-By)*(Ay-By)>0 && abs(i1-Bx)<abs(i2-By)))
                        BField[4][i1][i2] = 1;
                } break;
            case 7:
                if (!(i1==Bx && i2==By) && abs(i1-Bx)+abs(i2-By)<=step)
                {
                    if (abs(i1-Bx)<=abs(Ax-Bx) && abs(i2-By)<=abs(Ay-By))
                    {
                        if (abs(Ax-Bx)>abs(Ay-By) && Ax!=Bx &&
                            ((double)(i1-Bx)/(Ax-Bx))>0 &&
                            i2==(int)round((double)(i1-Bx)*(Ay-By)/(Ax-Bx))+By)
                            BField[4][i1][i2] = 1;
                        else if (abs(Ax-Bx)<=abs(Ay-By) && Ay!=By &&
                            ((double)(i2-By)/(Ay-By))>0 &&
                            i1==(int)round((double)(i2-By)*(Ax-Bx)/(Ay-By))+Bx)
                            BField[4][i1][i2] = 1;
                    }
                } break;
            }
        }
}

// ==== PlayMagicAmination ====
void PlayMagicAmination(int bnum, int bigami, int enum_, int level)
{
    char numbuf[8]; snprintf(numbuf, sizeof(numbuf), "%03d", enum_);
    std::string filename = AppPath + "eft/eft" + numbuf + ".pic";
    FILE* f = PasOpenB(filename, "rb");
    if (f)
    {
        int count = 0;
        fread(&count, 4, 1, f);
        for (int i = 0; i < count; i++)
        {
            SDL_PollEvent(nullptr);
            DrawBFieldWithEft(f, i, bigami, level);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }
        fclose(f);
    }
}

// ==== PlayActionAmination ====
void PlayActionAmination(int bnum, int mode)
{
    int d1 = Ax-Bx, d2 = Ay-By, dm = abs(d1)-abs(d2);
    if (dm > 0) BRole[bnum].Face = (d1 < 0) ? 0 : 3;
    else if (dm < 0) BRole[bnum].Face = (d2 < 0) ? 2 : 1;
    Redraw();
    int rnum = BRole[bnum].rnum;
    char rolestr[8]; snprintf(rolestr, sizeof(rolestr), "%03d", RRole[rnum].HeadNum);
    char modestr[8]; snprintf(modestr, sizeof(modestr), "%02d", mode);
    std::string filename = AppPath + "fight/" + rolestr + "/" + modestr + ".pic";
    FILE* f = PasOpenB(filename, "rb");
    if (f)
    {
        int count = 0;
        fread(&count, 4, 1, f);
        int beginpic = BRole[bnum].Face * (count / 4);
        int endpic = beginpic + (count / 4) - 1;
        for (int i = beginpic; i <= endpic; i++)
        {
            SDL_PollEvent(nullptr);
            DrawBFieldWithAction(f, bnum, i);
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            SDL_Delay((40 * GameSpeed) / 10);
        }
        fclose(f);
    }
    else
    {
        for (int i1 = 0; i1 <= 4; i1++)
        {
            snprintf(modestr, sizeof(modestr), "%02d", i1);
            filename = AppPath + "fight/" + rolestr + "/" + modestr + ".pic";
            f = PasOpenB(filename, "rb");
            if (f)
            {
                int count = 0;
                fread(&count, 4, 1, f);
                int beginpic = BRole[bnum].Face * (count / 4);
                int endpic = beginpic + (count / 4) - 1;
                for (int ix = beginpic; ix <= endpic; ix++)
                {
                    SDL_PollEvent(nullptr);
                    DrawBFieldWithAction(f, bnum, ix);
                    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                    SDL_Delay((40 * GameSpeed) / 10);
                }
                fclose(f);
                break;
            }
        }
    }
}

// ==== ShowMagicName ====
void ShowMagicName(int mnum)
{
    Redraw();
    std::wstring str = GBKToUnicode((const char*)RMagic[mnum].Name);
    if ((int)str.size() > 6) str = str.substr(0, 6);
    int l = (int)str.size();
    DrawTextWithRect(const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(str.c_str())),
        CENTER_X - l * 10, CENTER_Y - 150, l * 20 - 14, ColColor(0x14), ColColor(0x16));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
}

// ==== SelectMagic ====
int SelectMagic(int bnum)
{
    int menustatus = 0, max_ = 0, menu = 0;
    MenuString.clear(); MenuEngString.clear();
    MenuString.resize(10); MenuEngString.resize(10);
    for (int i = 0; i < 10; i++)
    {
        int mnum = RRole[BRole[bnum].rnum].Magic[i];
        if (mnum > 0)
        {
            if (RMagic[mnum].MagicType != 5)
            {
                if (RMagic[mnum].NeedMP <= RRole[BRole[bnum].rnum].CurrentMP)
                {
                    int lv = RRole[BRole[bnum].rnum].MagLevel[i] / 100;
                    if ((BRole[bnum].Progress+1)/3 >= (RMagic[mnum].NeedProgress*10)*lv+100 || BattleMode==0 || RRole[BRole[bnum].rnum].Angry==100)
                    {
                        menustatus |= (1 << i);
                        MenuString[i] = GBKToUnicode((const char*)RMagic[mnum].Name);
                        char buf[16]; snprintf(buf, sizeof(buf), "%3d", RRole[BRole[bnum].rnum].MagLevel[i]/100+1);
                        MenuEngString[i] = std::wstring(buf, buf+strlen(buf));
                        max_++;
                    }
                }
            }
            else
            {
                menustatus |= (1 << i);
                MenuString[i] = GBKToUnicode((const char*)RMagic[mnum].Name);
                char buf[16]; snprintf(buf, sizeof(buf), "%3d", RRole[BRole[bnum].rnum].MagLevel[i]/100+1);
                MenuEngString[i] = std::wstring(buf, buf+strlen(buf));
                max_++;
            }
        }
    }
    max_--;
    Redraw(); menu = 0;
    ShowMagicMenu(bnum, menustatus, menu, max_);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) break;
            if (event.key.key == SDLK_ESCAPE) { menu = -1; break; }
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu--; if (menu<0) menu=max_; ShowMagicMenu(bnum, menustatus, menu, max_); }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu++; if (menu>max_) menu=0; ShowMagicMenu(bnum, menustatus, menu, max_); }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) break;
            if (event.button.button == SDL_BUTTON_RIGHT) { menu = -1; break; }
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm>=100 && xm<267 && ym>=50 && ym<max_*22+78)
            { int mp=menu; menu=(ym-52)/22; if(menu>max_)menu=max_; if(menu<0)menu=0; if(mp!=menu) ShowMagicMenu(bnum, menustatus, menu, max_); }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    event.key.key = 0; event.button.button = 0;
    int result = menu;
    if (result >= 0)
    {
        int p = 0;
        for (int i = 0; i < 10; i++)
            if ((menustatus & (1<<i)) > 0) { if (p == menu) { result = i; break; } p++; }
    }
    return result;
}

// ==== ShowMagicMenu ====
void ShowMagicMenu(int bnum, int menustatus, int menu, int max_)
{
    Redraw();
    DrawRectangle(100, 50, 167, max_*22+28, 0, ColColor(255), 30);
    int p = 0;
    for (int i = 0; i < 10; i++)
    {
        if ((menustatus & (1<<i)) > 0)
        {
            uint16_t* ms = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(MenuString[i].c_str()));
            uint16_t* es = reinterpret_cast<uint16_t*>(MenuEngString[i].data());
            int mnum = RRole[BRole[bnum].rnum].Magic[i];
            if (p == menu) {
                DrawShadowText(ms, 83, 53+22*p, ColColor(0x64), ColColor(0x66));
                if (RMagic[mnum].MagicType != 5)
                    DrawEngShadowText(es, 223, 53+22*p, ColColor(0x64), ColColor(0x66));
            } else {
                if (RMagic[mnum].MagicType == 5)
                    DrawShadowText(ms, 83, 53+22*p, ColColor(0x5), ColColor(0x7));
                else {
                    DrawShadowText(ms, 83, 53+22*p, ColColor(0x21), ColColor(0x23));
                    DrawEngShadowText(es, 223, 53+22*p, ColColor(0x21), ColColor(0x23));
                }
            }
            p++;
        }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== Attack ====
void Attack(int bnum)
{
    int msel = SelectMagic(bnum);
    if (msel < 0) return;
    int rnum = BRole[bnum].rnum;
    int mnum = RRole[rnum].Magic[msel];
    int level = RRole[rnum].MagLevel[msel] / 100 + 1;
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1], RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 1) step++;
    if (GetEquipState(rnum, 22) || GetGongtiState(rnum, 22)) step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    int AttAreaType = RMagic[mnum].AttAreaType;
    bool selected = false;
    switch (AttAreaType)
    {
    case 0: case 3: case 6:
        selected = SelectRange(bnum, AttAreaType, step, range); break;
    case 1: case 4: case 5:
        selected = SelectDirector(bnum, AttAreaType, step, range); break;
    case 2:
        selected = SelectCross(bnum, AttAreaType, step, range); break;
    case 7:
        selected = SelectLine(bnum, AttAreaType, step, range); break;
    default:
        selected = SelectFar(bnum, mnum, level); break;
    }
    if (selected)
        AttackAction(bnum, mnum, level);
}

// ==== AttackAction ====
void AttackAction(int bnum, int mnum, int level)
{
    int rnum = BRole[bnum].rnum;
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1], RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 1) step++;
    if (GetEquipState(rnum, 22) || GetGongtiState(rnum, 22)) step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    SetAminationPosition(RMagic[mnum].AttAreaType, step, range);
    ShowMagicName(mnum);
    PlayActionAmination(bnum, RMagic[mnum].MagicType);
    PlayMagicAmination(bnum, RMagic[mnum].bigami, RMagic[mnum].AmiNum, level);
    CalHurtRole(bnum, mnum, level);
    BRole[bnum].Acted = 1;
    RRole[rnum].CurrentMP -= RMagic[mnum].NeedMP;
    RRole[rnum].PhyPower -= 10;
    if (BattleMode > 0)
    {
        int lv = RRole[rnum].MagLevel[(int)(std::find(RRole[rnum].Magic, RRole[rnum].Magic+10, mnum) - RRole[rnum].Magic)] / 100;
        BRole[bnum].Progress -= static_cast<int16_t>(300 + (RMagic[mnum].NeedProgress * 10) * lv + 100);
    }
    if (RRole[rnum].Angry >= 100) RRole[rnum].Angry = 0;
}

// ==== CalHurtRole ====
void CalHurtRole(int bnum, int mnum, int level)
{
    for (int i = 0; i < BROLE_COUNT; i++) BRole[i].ShowNumber = -1;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BField[4][BRole[i].X][BRole[i].Y] > 0)
        {
            int rnum_i = BRole[i].rnum;
            if (RMagic[mnum].HurtType == 0) // 普通
            {
                int h = CalHurtValue(bnum, i, mnum, level);
                // 乾坤大挪移
                if (GetEquipState(rnum_i, 6) || GetGongtiState(rnum_i, 6))
                    i = ReMoveHurt(i, bnum);
                // 反噬
                if (GetEquipState(rnum_i, 7) || GetGongtiState(rnum_i, 7))
                    i = RetortHurt(i, bnum);
                // 闪避
                if (BRole[i].PerfectDodge > 0) h = 0;
                else if (BRole[i].AddDodge > 0 && rand() % 100 < 30 + BRole[i].AddDodge * 3) h = 0;
                else if (rand() % 100 < GetRoleSpeed(rnum_i, true) - GetRoleSpeed(BRole[bnum].rnum, true) + 10) h = 0;
                if (h > 0)
                {
                    RRole[rnum_i].CurrentHP -= static_cast<int16_t>(h);
                    if (RRole[rnum_i].CurrentHP < 0) RRole[rnum_i].CurrentHP = 0;
                    BRole[bnum].ExpGot += static_cast<int16_t>(h);
                }
                BRole[i].ShowNumber = static_cast<int16_t>(h);
            }
            else if (RMagic[mnum].HurtType == 1) // 治疗
            {
                if (BRole[i].Team == BRole[bnum].Team)
                {
                    int h = CalHurtValue(bnum, i, mnum, level);
                    RRole[rnum_i].CurrentHP += static_cast<int16_t>(h);
                    if (RRole[rnum_i].CurrentHP > RRole[rnum_i].MaxHP)
                        RRole[rnum_i].CurrentHP = RRole[rnum_i].MaxHP;
                    BRole[i].ShowNumber = static_cast<int16_t>(h);
                }
            }
        }
    }
    ShowHurtValue((RMagic[mnum].HurtType == 0) ? 0 : 3);
    ClearDeadRolePic();
}

// ==== CalHurtValue ====
int CalHurtValue(int bnum1, int bnum2, int mnum, int level)
{
    int r1 = BRole[bnum1].rnum, r2 = BRole[bnum2].rnum;
    int att = RRole[r1].Attack;
    int def = RRole[r2].Defence;
    int spd1 = GetRoleSpeed(r1, true), spd2 = GetRoleSpeed(r2, true);
    int power = CalNewHurtValue(level, RMagic[mnum].AddHP[0], RMagic[mnum].AddHP[2], 100);
    if (power == 0) power = 1;
    int base = att - def + power;
    if (base < 1) base = 1;
    // 武器匹配加成
    int weaponMatch = 0;
    if (RRole[r1].Equip[0] >= 0)
    {
        int etype = RItem[RRole[r1].Equip[0]].ItemType;
        if (etype == RMagic[mnum].MagicType || RMagic[mnum].MagicType == 5) weaponMatch = 10;
    }
    int result = base * (100 + weaponMatch + rand() % 20 - 10) / 100;
    if (result < 1) result = 1;
    // 知识加成
    if (BRole[bnum1].Knowledge > 0 && GetPetSkill(3, 3))
        result = result * (100 + BRole[bnum1].Knowledge) / 100;
    return result;
}

// ==== BattleMenuItem ====
void BattleMenuItem(int bnum)
{
    int rnum = BRole[bnum].rnum;
    int inum = CommonMenu2(100, 50, 200);
    if (inum < 0) return;
    if (RItem[inum].ItemType == 3)
    {
        EatOneItem(rnum, inum);
        instruct_32(inum, 1);
        BRole[bnum].Acted = 1;
    }
    else if (RItem[inum].ItemType == 4)
    {
        UseHiddenWeapen(bnum, inum);
    }
}

// ==== UsePoision ====
void UsePoision(int bnum)
{
    int step = GetRoleUsePoi(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, step);
    if (SelectAim(bnum, step))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].X == Ax && BRole[i].Y == Ay)
            {
                int poi = GetRoleUsePoi(BRole[bnum].rnum, true) - GetRoleDefPoi(BRole[i].rnum, true);
                if (poi > 0)
                {
                    // 乾坤大挪移
                    int target = i;
                    if (GetEquipState(BRole[i].rnum, 6) || GetGongtiState(BRole[i].rnum, 6))
                        target = ReMoveHurt(i, bnum);
                    // 反噬
                    if (GetEquipState(BRole[i].rnum, 7) || GetGongtiState(BRole[i].rnum, 7))
                        target = RetortHurt(i, bnum);
                    RRole[BRole[target].rnum].Poision += static_cast<int16_t>(poi);
                    if (RRole[BRole[target].rnum].Poision > 100) RRole[BRole[target].rnum].Poision = 100;
                }
                break;
            }
        }
        BRole[bnum].Acted = 1;
        RRole[BRole[bnum].rnum].PhyPower -= 30;
    }
}

// ==== Medcine ====
void Medcine(int bnum)
{
    int step = GetRoleMedcine(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, step);
    if (SelectAim(bnum, step))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].X == Ax && BRole[i].Y == Ay)
            {
                int heal = GetRoleMedcine(BRole[bnum].rnum, true) * 5;
                // Pet skill: AoE heal check
                if (GetPetSkill(6, 3) && BRole[bnum].rnum == 0)
                {
                    for (int j = 0; j < BROLE_COUNT; j++)
                    {
                        if (BRole[j].Dead == 0 && BRole[j].rnum >= 0 && BRole[j].Team == BRole[bnum].Team &&
                            abs(BRole[j].X - BRole[i].X) + abs(BRole[j].Y - BRole[i].Y) <= 3)
                        {
                            RRole[BRole[j].rnum].CurrentHP += static_cast<int16_t>(heal);
                            if (RRole[BRole[j].rnum].CurrentHP > RRole[BRole[j].rnum].MaxHP)
                                RRole[BRole[j].rnum].CurrentHP = RRole[BRole[j].rnum].MaxHP;
                            RRole[BRole[j].rnum].Hurt -= static_cast<int16_t>(GetRoleMedcine(BRole[bnum].rnum, true) / 10);
                            if (RRole[BRole[j].rnum].Hurt < 0) RRole[BRole[j].rnum].Hurt = 0;
                        }
                    }
                }
                else
                {
                    RRole[BRole[i].rnum].CurrentHP += static_cast<int16_t>(heal);
                    if (RRole[BRole[i].rnum].CurrentHP > RRole[BRole[i].rnum].MaxHP)
                        RRole[BRole[i].rnum].CurrentHP = RRole[BRole[i].rnum].MaxHP;
                    RRole[BRole[i].rnum].Hurt -= static_cast<int16_t>(GetRoleMedcine(BRole[bnum].rnum, true) / 10);
                    if (RRole[BRole[i].rnum].Hurt < 0) RRole[BRole[i].rnum].Hurt = 0;
                }
                break;
            }
        }
        BRole[bnum].Acted = 1;
        RRole[BRole[bnum].rnum].PhyPower -= 50;
    }
}

// ==== MedFrozen ====
void MedFrozen(int bnum)
{
    int step = GetRoleMedcine(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, step);
    if (SelectAim(bnum, step))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].X == Ax && BRole[i].Y == Ay)
            {
                BRole[i].frozen = 0;
                break;
            }
        }
        BRole[bnum].Acted = 1;
        RRole[BRole[bnum].rnum].PhyPower -= 50;
        RRole[BRole[bnum].rnum].CurrentMP -= static_cast<int16_t>(GetRoleMedcine(BRole[bnum].rnum, true) * 5);
    }
}

// ==== MedPoision ====
void MedPoision(int bnum)
{
    int step = GetRoleMedPoi(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, step);
    if (SelectAim(bnum, step))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].X == Ax && BRole[i].Y == Ay)
            {
                int med = GetRoleMedPoi(BRole[bnum].rnum, true);
                // Pet skill: AoE cure poison check
                if (GetPetSkill(6, 3) && BRole[bnum].rnum == 0)
                {
                    for (int j = 0; j < BROLE_COUNT; j++)
                    {
                        if (BRole[j].Dead == 0 && BRole[j].rnum >= 0 && BRole[j].Team == BRole[bnum].Team &&
                            abs(BRole[j].X - BRole[i].X) + abs(BRole[j].Y - BRole[i].Y) <= 3)
                        {
                            RRole[BRole[j].rnum].Poision -= static_cast<int16_t>(med);
                            if (RRole[BRole[j].rnum].Poision < 0) RRole[BRole[j].rnum].Poision = 0;
                        }
                    }
                }
                else
                {
                    RRole[BRole[i].rnum].Poision -= static_cast<int16_t>(med);
                    if (RRole[BRole[i].rnum].Poision < 0) RRole[BRole[i].rnum].Poision = 0;
                }
                break;
            }
        }
        BRole[bnum].Acted = 1;
        RRole[BRole[bnum].rnum].PhyPower -= 50;
    }
}

// ==== UseHiddenWeapen ====
void UseHiddenWeapen(int bnum, int inum)
{
    int step = GetRoleHidWeapon(BRole[bnum].rnum, true) / 10;
    if (step < 1) step = 1;
    CalCanSelect(bnum, 0, step);
    if (SelectAim(bnum, step))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].X == Ax && BRole[i].Y == Ay)
            {
                int h = GetRoleHidWeapon(BRole[bnum].rnum, true) + RItem[inum].AddCurrentHP;
                if (h > 0)
                {
                    RRole[BRole[i].rnum].CurrentHP -= static_cast<int16_t>(h);
                    if (RRole[BRole[i].rnum].CurrentHP < 0) RRole[BRole[i].rnum].CurrentHP = 0;
                }
                if (RItem[inum].AddUsePoi > 0)
                {
                    RRole[BRole[i].rnum].Poision += RItem[inum].AddUsePoi;
                    if (RRole[BRole[i].rnum].Poision > 100) RRole[BRole[i].rnum].Poision = 100;
                }
                BRole[i].ShowNumber = static_cast<int16_t>(h);
                break;
            }
        }
        ShowHurtValue(0);
        instruct_32(inum, 1);
        BRole[bnum].Acted = 1;
    }
}

// ==== AutoBattle ====
void AutoBattle(int bnum) { AutoBattle2(bnum); }

// ==== AutoUseItem ====
void AutoUseItem(int bnum, int list)
{
    int rnum = BRole[bnum].rnum;
    int bestVal = 0, bestItem = -1;
    for (int i = 0; i < (int)RItemList.size(); i++)
    {
        if (RItemList[i].Amount > 0 && RItem[RItemList[i].Number].ItemType == 3)
        {
            int val = 0;
            if (list == 0) val = RItem[RItemList[i].Number].AddCurrentHP;
            else if (list == 1) val = RItem[RItemList[i].Number].AddCurrentMP;
            if (val > bestVal) { bestVal = val; bestItem = RItemList[i].Number; }
        }
    }
    if (bestItem >= 0)
    {
        EatOneItem(rnum, bestItem);
        instruct_32(bestItem, 1);
        BRole[bnum].Acted = 1;
    }
}

// ==== PetEffect ====
void PetEffect()
{
    // Pet effects: collection (2,1), theft (2,3)
    if (GetPetSkill(2, 1) || GetPetSkill(2, 3))
    {
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead == 1 && BRole[i].rnum >= 0 && BRole[i].Team == 1)
            {
                for (int k = 0; k < 4; k++)
                {
                    if (RRole[BRole[i].rnum].Equip[k] >= 0)
                    {
                        if (rand() % 100 < 20)
                        {
                            instruct_32(RRole[BRole[i].rnum].Equip[k], -1);
                            RRole[BRole[i].rnum].Equip[k] = -1;
                        }
                    }
                }
            }
        }
    }
}

// ==== AutoBattle2 ====
void AutoBattle2(int bnum)
{
    int rnum = BRole[bnum].rnum;
    // Try healing low HP ally
    if (GetRoleMedcine(rnum, true) > 0 && RRole[rnum].PhyPower >= 50)
    {
        int Mx1, My1, Ax1, Ay1;
        trymovecure(Mx1, My1, Ax1, Ay1, bnum);
        if (Ax1 >= 0)
        {
            // Move
            if (BField[2][Bx][By] == bnum) BField[2][Bx][By] = -1;
            Bx = Mx1; By = My1;
            BRole[bnum].X = static_cast<int16_t>(Bx); BRole[bnum].Y = static_cast<int16_t>(By);
            BField[2][Bx][By] = static_cast<int16_t>(bnum);
            Ax = Ax1; Ay = Ay1;
            cureaction(bnum);
            return;
        }
    }
    // Try attacking with best magic
    int bestMagic = -1, bestLevel = 0, maxHurt = 0;
    int bestMx = Bx, bestMy = By, bestAx = Bx, bestAy = By;
    for (int i = 0; i < 10; i++)
    {
        int mnum = RRole[rnum].Magic[i];
        if (mnum > 0 && RMagic[mnum].MagicType != 5 && RMagic[mnum].NeedMP <= RRole[rnum].CurrentMP && RRole[rnum].PhyPower >= 10)
        {
            int level = RRole[rnum].MagLevel[i] / 100 + 1;
            int Mx1, My1, Ax1, Ay1, tempmax;
            trymoveattack(Mx1, My1, Ax1, Ay1, tempmax, bnum, mnum, level);
            if (tempmax > maxHurt)
            {
                maxHurt = tempmax; bestMagic = mnum; bestLevel = level;
                bestMx = Mx1; bestMy = My1; bestAx = Ax1; bestAy = Ay1;
            }
        }
    }
    if (bestMagic >= 0 && maxHurt > 0)
    {
        if (BField[2][Bx][By] == bnum) BField[2][Bx][By] = -1;
        Bx = bestMx; By = bestMy;
        BRole[bnum].X = static_cast<int16_t>(Bx); BRole[bnum].Y = static_cast<int16_t>(By);
        BField[2][Bx][By] = static_cast<int16_t>(bnum);
        Ax = bestAx; Ay = bestAy;
        AttackAction(bnum, bestMagic, bestLevel);
        return;
    }
    // Move towards nearest enemy
    int Mx1n, My1n;
    nearestmove(Mx1n, My1n, bnum);
    if (BField[2][Bx][By] == bnum) BField[2][Bx][By] = -1;
    Bx = Mx1n; By = My1n;
    BRole[bnum].X = static_cast<int16_t>(Bx); BRole[bnum].Y = static_cast<int16_t>(By);
    BField[2][Bx][By] = static_cast<int16_t>(bnum);
    Rest(bnum);
}

// ==== Auto ====
void Auto(int bnum)
{
    int mode = SelectAutoMode();
    if (mode == 0) BRole[bnum].Auto = 0;
    else if (mode == 1)
    {
        if (TeamModeMenu())
        {
            for (int i = 0; i < BROLE_COUNT; i++)
                if (BRole[i].Team == 0 && BRole[i].Dead == 0 && BRole[i].rnum >= 0)
                    if (BRole[i].Auto == -1) BRole[i].Auto = 0;
        }
    }
}

// ==== trymoveattack ====
void trymoveattack(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int bnum, int mnum, int level)
{
    tempmaxhurt = 0;
    Mx1 = Bx; My1 = By; Ax1 = Bx; Ay1 = By;
    CalCanSelect(bnum, 0, BRole[bnum].Step);
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1) step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22)) step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    for (int mx = std::max(0, Bx - BRole[bnum].Step); mx <= std::min(63, Bx + BRole[bnum].Step); mx++)
        for (int my = std::max(0, By - BRole[bnum].Step); my <= std::min(63, By + BRole[bnum].Step); my++)
        {
            if (BField[3][mx][my] < 0) continue;
            // For each possible aim position
            switch (RMagic[mnum].AttAreaType)
            {
            case 0: case 3: case 6:
                calarea(Mx1, My1, Ax1, Ay1, tempmaxhurt, mx, my, bnum, mnum, level); break;
            case 1:
                calline(Mx1, My1, Ax1, Ay1, tempmaxhurt, mx, my, bnum, mnum, level); break;
            case 7:
                calNewline(Mx1, My1, Ax1, Ay1, tempmaxhurt, mx, my, bnum, mnum, level); break;
            default:
                calpoint(Mx1, My1, Ax1, Ay1, tempmaxhurt, mx, my, bnum, mnum, level); break;
            }
        }
}

// ==== calline ====
void calline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1) step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22)) step++;
    int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (int d = 0; d < 4; d++)
    {
        int hurt = 0;
        for (int s = 1; s <= step; s++)
        {
            int tx = curX + dirs[d][0] * s, ty = curY + dirs[d][1] * s;
            if (tx<0||tx>63||ty<0||ty>63) break;
            for (int i = 0; i < BROLE_COUNT; i++)
                if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].X==tx && BRole[i].Y==ty && BRole[i].Team!=BRole[bnum].Team)
                    hurt += CalHurtValue(bnum, i, mnum, level);
        }
        if (hurt > tempmaxhurt)
        { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = curX+dirs[d][0]; Ay1 = curY+dirs[d][1]; }
    }
}

// ==== calarea ====
void calarea(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1) step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22)) step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    for (int ax = std::max(0, curX - step); ax <= std::min(63, curX + step); ax++)
        for (int ay = std::max(0, curY - step); ay <= std::min(63, curY + step); ay++)
        {
            if (abs(ax-curX)+abs(ay-curY) > step) continue;
            int hurt = 0;
            for (int i = 0; i < BROLE_COUNT; i++)
            {
                if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
                {
                    bool inRange = false;
                    if (RMagic[mnum].AttAreaType == 3)
                        inRange = (abs(BRole[i].X-ax)<=range && abs(BRole[i].Y-ay)<=range);
                    else
                        inRange = (abs(BRole[i].X-ax)+abs(BRole[i].Y-ay)<=range);
                    if (inRange) hurt += CalHurtValue(bnum, i, mnum, level);
                }
            }
            if (hurt > tempmaxhurt)
            { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = ax; Ay1 = ay; }
        }
}

// ==== calNewline ====
void calNewline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1) step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22)) step++;
    for (int ax = std::max(0, curX - step); ax <= std::min(63, curX + step); ax++)
        for (int ay = std::max(0, curY - step); ay <= std::min(63, curY + step); ay++)
        {
            if (abs(ax-curX)+abs(ay-curY) > step) continue;
            if (ax == curX && ay == curY) continue;
            int hurt = 0;
            // calculate line from (curX,curY) toward (ax,ay)
            for (int i = 0; i < BROLE_COUNT; i++)
            {
                if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
                {
                    int bix = BRole[i].X, biy = BRole[i].Y;
                    if (abs(bix-curX)+abs(biy-curY) <= step && abs(bix-curX)<=abs(ax-curX) && abs(biy-curY)<=abs(ay-curY))
                    {
                        // Check if on the line
                        bool onLine = false;
                        if (abs(ax-curX) > abs(ay-curY) && ax!=curX)
                        {
                            if ((double)(bix-curX)/(ax-curX)>0 &&
                                biy==(int)round((double)(bix-curX)*(ay-curY)/(ax-curX))+curY)
                                onLine = true;
                        }
                        else if (ay!=curY)
                        {
                            if ((double)(biy-curY)/(ay-curY)>0 &&
                                bix==(int)round((double)(biy-curY)*(ax-curX)/(ay-curY))+curX)
                                onLine = true;
                        }
                        if (onLine) hurt += CalHurtValue(bnum, i, mnum, level);
                    }
                }
            }
            if (hurt > tempmaxhurt)
            { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = ax; Ay1 = ay; }
        }
}

// ==== calpoint ====
void calpoint(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    if (CheckEquipSet(RRole[BRole[bnum].rnum].Equip[0], RRole[BRole[bnum].rnum].Equip[1],
        RRole[BRole[bnum].rnum].Equip[2], RRole[BRole[bnum].rnum].Equip[3]) == 1) step++;
    if (GetEquipState(BRole[bnum].rnum, 22) || GetGongtiState(BRole[bnum].rnum, 22)) step++;
    int range = RMagic[mnum].AttDistance[level - 1];
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            int dist = abs(BRole[i].X - curX) + abs(BRole[i].Y - curY);
            if (dist <= step && dist >= RMagic[mnum].MinStep)
            {
                int hurt = CalHurtValue(bnum, i, mnum, level);
                if (hurt > tempmaxhurt)
                { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = BRole[i].X; Ay1 = BRole[i].Y; }
            }
        }
    }
}

// ==== calcross ====
void calcross(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    int range = RMagic[mnum].AttDistance[level - 1];
    int hurt = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            if (abs(BRole[i].X-curX)+abs(BRole[i].Y-curY) <= range)
                hurt += CalHurtValue(bnum, i, mnum, level);
        }
    }
    if (hurt > tempmaxhurt)
    { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = curX; Ay1 = curY; }
}

// ==== caldirdiamond ====
void caldirdiamond(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (int d = 0; d < 4; d++)
    {
        int hurt = 0;
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
            {
                int dx = BRole[i].X - curX, dy = BRole[i].Y - curY;
                if (abs(dx)+abs(dy) <= step && abs(dx)!=abs(dy))
                {
                    if ((dx*dirs[d][0]>0 && abs(dx)>abs(dy)) || (dy*dirs[d][1]>0 && abs(dx)<abs(dy)))
                        hurt += CalHurtValue(bnum, i, mnum, level);
                }
            }
        }
        if (hurt > tempmaxhurt)
        { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = curX+dirs[d][0]; Ay1 = curY+dirs[d][1]; }
    }
}

// ==== caldirangle ====
void caldirangle(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (int d = 0; d < 4; d++)
    {
        int hurt = 0;
        for (int i = 0; i < BROLE_COUNT; i++)
        {
            if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
            {
                int dx = BRole[i].X - curX, dy = BRole[i].Y - curY;
                if (abs(dx)<=step && abs(dy)<=step && abs(dx)!=abs(dy))
                {
                    if ((dx*dirs[d][0]>0 && abs(dx)>abs(dy)) || (dy*dirs[d][1]>0 && abs(dx)<abs(dy)))
                        hurt += CalHurtValue(bnum, i, mnum, level);
                }
            }
        }
        if (hurt > tempmaxhurt)
        { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = curX+dirs[d][0]; Ay1 = curY+dirs[d][1]; }
    }
}

// ==== calfar ====
void calfar(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level)
{
    int step = RMagic[mnum].MoveDistance[level - 1];
    int minstep = RMagic[mnum].MinStep;
    int range = RMagic[mnum].AttDistance[level - 1];
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            int dist = abs(BRole[i].X - curX) + abs(BRole[i].Y - curY);
            if (dist <= step && dist > minstep)
            {
                int hurt = CalHurtValue(bnum, i, mnum, level);
                if (hurt > tempmaxhurt)
                { tempmaxhurt = hurt; Mx1 = curX; My1 = curY; Ax1 = BRole[i].X; Ay1 = BRole[i].Y; }
            }
        }
    }
}

// ==== nearestmove ====
void nearestmove(int& Mx, int& My, int bnum)
{
    Mx = Bx; My = By;
    int mindist = 9999;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            int dist = abs(BRole[i].X - Bx) + abs(BRole[i].Y - By);
            if (dist < mindist)
            {
                mindist = dist;
                // BFS: find movable cell closest to enemy
                int bestd = 9999;
                for (int mx = std::max(0,Bx-BRole[bnum].Step); mx <= std::min(63,Bx+BRole[bnum].Step); mx++)
                    for (int my = std::max(0,By-BRole[bnum].Step); my <= std::min(63,By+BRole[bnum].Step); my++)
                    {
                        if (BField[3][mx][my] >= 0)
                        {
                            int d = abs(mx-BRole[i].X)+abs(my-BRole[i].Y);
                            if (d < bestd) { bestd = d; Mx = mx; My = my; }
                        }
                    }
            }
        }
    }
}

// ==== farthestmove ====
void farthestmove(int& Mx, int& My, int bnum)
{
    Mx = Bx; My = By;
    int maxdist = -1;
    for (int mx = std::max(0,Bx-BRole[bnum].Step); mx <= std::min(63,Bx+BRole[bnum].Step); mx++)
        for (int my = std::max(0,By-BRole[bnum].Step); my <= std::min(63,By+BRole[bnum].Step); my++)
        {
            if (BField[3][mx][my] >= 0)
            {
                int mindist = 9999;
                for (int i = 0; i < BROLE_COUNT; i++)
                    if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
                    {
                        int d = abs(mx-BRole[i].X)+abs(my-BRole[i].Y);
                        if (d < mindist) mindist = d;
                    }
                if (mindist > maxdist) { maxdist = mindist; Mx = mx; My = my; }
            }
        }
}

// ==== trymovecure ====
void trymovecure(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum)
{
    Ax1 = -1;
    int healValue = GetRoleMedcine(BRole[bnum].rnum, true);
    int step = healValue / 15 + 1;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team==BRole[bnum].Team && i!=bnum &&
            RRole[BRole[i].rnum].CurrentHP < RRole[BRole[i].rnum].MaxHP / 2)
        {
            // Can we reach a cell within heal step of target?
            CalCanSelect(bnum, 0, BRole[bnum].Step);
            for (int mx = std::max(0,BRole[i].X-step); mx <= std::min(63,BRole[i].X+step); mx++)
                for (int my = std::max(0,BRole[i].Y-step); my <= std::min(63,BRole[i].Y+step); my++)
                {
                    if (abs(mx-BRole[i].X)+abs(my-BRole[i].Y) <= step && BField[3][mx][my] >= 0)
                    {
                        Mx1 = mx; My1 = my; Ax1 = BRole[i].X; Ay1 = BRole[i].Y;
                        return;
                    }
                }
        }
    }
}

// ==== cureaction ====
void cureaction(int bnum)
{
    int heal = GetRoleMedcine(BRole[bnum].rnum, true) * 5;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].X==Ax && BRole[i].Y==Ay)
        {
            RRole[BRole[i].rnum].CurrentHP += static_cast<int16_t>(heal);
            if (RRole[BRole[i].rnum].CurrentHP > RRole[BRole[i].rnum].MaxHP)
                RRole[BRole[i].rnum].CurrentHP = RRole[BRole[i].rnum].MaxHP;
            RRole[BRole[i].rnum].Hurt -= static_cast<int16_t>(GetRoleMedcine(BRole[bnum].rnum, true) / 10);
            if (RRole[BRole[i].rnum].Hurt < 0) RRole[BRole[i].rnum].Hurt = 0;
            break;
        }
    }
    BRole[bnum].Acted = 1;
    RRole[BRole[bnum].rnum].PhyPower -= 50;
}

// ==== trymoveHidden ====
void trymoveHidden(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum, int inum)
{
    Ax1 = -1;
    int step = GetRoleHidWeapon(BRole[bnum].rnum, true) / 10;
    if (step < 1) step = 1;
    CalCanSelect(bnum, 0, BRole[bnum].Step);
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            for (int mx = std::max(0,BRole[i].X-step); mx <= std::min(63,BRole[i].X+step); mx++)
                for (int my = std::max(0,BRole[i].Y-step); my <= std::min(63,BRole[i].Y+step); my++)
                {
                    if (abs(mx-BRole[i].X)+abs(my-BRole[i].Y) <= step && BField[3][mx][my] >= 0)
                    { Mx1=mx; My1=my; Ax1=BRole[i].X; Ay1=BRole[i].Y; return; }
                }
        }
    }
}

// ==== Hiddenaction ====
void Hiddenaction(int bnum, int inum)
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].X==Ax && BRole[i].Y==Ay)
        {
            int h = GetRoleHidWeapon(BRole[bnum].rnum, true) + RItem[inum].AddCurrentHP;
            if (h > 0) { RRole[BRole[i].rnum].CurrentHP -= static_cast<int16_t>(h); if (RRole[BRole[i].rnum].CurrentHP < 0) RRole[BRole[i].rnum].CurrentHP = 0; }
            if (RItem[inum].AddUsePoi > 0) { RRole[BRole[i].rnum].Poision += RItem[inum].AddUsePoi; if (RRole[BRole[i].rnum].Poision > 100) RRole[BRole[i].rnum].Poision = 100; }
            BRole[i].ShowNumber = static_cast<int16_t>(h);
            break;
        }
    }
    ShowHurtValue(0);
    instruct_32(inum, 1);
    BRole[bnum].Acted = 1;
}

// ==== trymoveUsePoi ====
void trymoveUsePoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum)
{
    Ax1 = -1;
    int step = GetRoleUsePoi(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, BRole[bnum].Step);
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team!=BRole[bnum].Team)
        {
            for (int mx = std::max(0,BRole[i].X-step); mx <= std::min(63,BRole[i].X+step); mx++)
                for (int my = std::max(0,BRole[i].Y-step); my <= std::min(63,BRole[i].Y+step); my++)
                {
                    if (abs(mx-BRole[i].X)+abs(my-BRole[i].Y) <= step && BField[3][mx][my] >= 0)
                    { Mx1=mx; My1=my; Ax1=BRole[i].X; Ay1=BRole[i].Y; return; }
                }
        }
    }
}

// ==== UsePoiaction ====
void UsePoiaction(int bnum)
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].X==Ax && BRole[i].Y==Ay)
        {
            int poi = GetRoleUsePoi(BRole[bnum].rnum, true) - GetRoleDefPoi(BRole[i].rnum, true);
            if (poi > 0) { RRole[BRole[i].rnum].Poision += static_cast<int16_t>(poi); if (RRole[BRole[i].rnum].Poision > 100) RRole[BRole[i].rnum].Poision = 100; }
            break;
        }
    }
    BRole[bnum].Acted = 1;
    RRole[BRole[bnum].rnum].PhyPower -= 30;
}

// ==== trymoveMedPoi ====
void trymoveMedPoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum)
{
    Ax1 = -1;
    int step = GetRoleMedPoi(BRole[bnum].rnum, true) / 15 + 1;
    CalCanSelect(bnum, 0, BRole[bnum].Step);
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team==BRole[bnum].Team &&
            RRole[BRole[i].rnum].Poision > 30)
        {
            for (int mx = std::max(0,BRole[i].X-step); mx <= std::min(63,BRole[i].X+step); mx++)
                for (int my = std::max(0,BRole[i].Y-step); my <= std::min(63,BRole[i].Y+step); my++)
                {
                    if (abs(mx-BRole[i].X)+abs(my-BRole[i].Y) <= step && BField[3][mx][my] >= 0)
                    { Mx1=mx; My1=my; Ax1=BRole[i].X; Ay1=BRole[i].Y; return; }
                }
        }
    }
}

// ==== MedPoiaction ====
void MedPoiaction(int bnum)
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].X==Ax && BRole[i].Y==Ay)
        {
            int med = GetRoleMedPoi(BRole[bnum].rnum, true);
            RRole[BRole[i].rnum].Poision -= static_cast<int16_t>(med);
            if (RRole[BRole[i].rnum].Poision < 0) RRole[BRole[i].rnum].Poision = 0;
            break;
        }
    }
    BRole[bnum].Acted = 1;
    RRole[BRole[bnum].rnum].PhyPower -= 50;
}

// ==== ShowModeMenu ====
void ShowModeMenu(int menu)
{
    std::wstring word[3] = { L" 瘋子", L" 傻子", L" 呆子" };
    Redraw();
    DrawRectangle(CENTER_X - 45, 100, 90, 94, 0, ColColor(255), 30);
    for (int i = 0; i < 3; i++)
    {
        uint16_t* w = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(word[i].c_str()));
        if (i == menu) DrawShadowText(w, CENTER_X - 55, 103 + 22*i, ColColor(0x64), ColColor(0x66));
        else DrawShadowText(w, CENTER_X - 55, 103 + 22*i, ColColor(0x21), ColColor(0x23));
    }
    SDL_UpdateRect2(screen, CENTER_X-45, 100, 91, 95);
}

// ==== SelectAutoMode ====
int SelectAutoMode()
{
    int menu = 0;
    ShowModeMenu(menu);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return menu; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return -1; }
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu--; if (menu<0) menu=2; ShowModeMenu(menu); }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu++; if (menu>2) menu=0; ShowModeMenu(menu); }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return menu; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return -1; }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return -1;
}

// ==== ShowTeamModeMenu ====
void ShowTeamModeMenu(int menu)
{
    Redraw();
    int y = 100;
    DrawRectangle(CENTER_X - 80, y, 160, BROLE_COUNT * 22 + 28, 0, ColColor(255), 30);
    int p = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead == 0 && BRole[i].rnum >= 0 && BRole[i].Team == 0)
        {
            std::wstring name = GBKToUnicode((const char*)RRole[BRole[i].rnum].Name);
            uint16_t* w = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(name.c_str()));
            if (p == menu) DrawShadowText(w, CENTER_X - 70, y + 3 + 22*p, ColColor(0x64), ColColor(0x66));
            else DrawShadowText(w, CENTER_X - 70, y + 3 + 22*p, ColColor(0x21), ColColor(0x23));
            p++;
        }
    }
    SDL_UpdateRect2(screen, CENTER_X-80, y, 161, p*22+29);
}

// ==== TeamModeMenu ====
bool TeamModeMenu()
{
    int menu = 0, max_ = 0;
    for (int i = 0; i < BROLE_COUNT; i++)
        if (BRole[i].Dead==0 && BRole[i].rnum>=0 && BRole[i].Team==0) max_++;
    max_--;
    ShowTeamModeMenu(menu);
    while (SDL_PollEvent(&event) || true)
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { event.key.key = 0; return true; }
            if (event.key.key == SDLK_ESCAPE) { event.key.key = 0; return false; }
        }
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu--; if (menu<0) menu=max_; ShowTeamModeMenu(menu); }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu++; if (menu>max_) menu=0; ShowTeamModeMenu(menu); }
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_LEFT) { event.button.button = 0; return true; }
            if (event.button.button == SDL_BUTTON_RIGHT) { event.button.button = 0; return false; }
        }
        event.key.key = 0; event.button.button = 0;
        SDL_Delay((20 * GameSpeed) / 10);
    }
    return false;
}
