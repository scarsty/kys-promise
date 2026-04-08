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
    // 乾坤大挪移
    // TODO: 完整实现
    return 0;
}

// ==== RetortHurt ====
int16_t RetortHurt(int bnum, int AttackBnum)
{
    // 斗转星移
    // TODO: 完整实现
    return 0;
}

// ==== CalPoiHurtLife ====
void CalPoiHurtLife(int bnum)
{
    // TODO: 完整实现
}

// ==== ClearDeadRolePic ====
void ClearDeadRolePic()
{
    for (int i = 0; i < BROLE_COUNT; i++)
    {
        if (BRole[i].Dead == 1 && BRole[i].rnum >= 0)
        {
            BField[2][BRole[i].X][BRole[i].Y] = -1;
        }
    }
}

// ==== Wait ====
void Wait(int bnum)
{
    BRole[bnum].Acted = 1;
    BRole[bnum].wait = 1;
    BRole[bnum].Progress -= 100;
}

// ==== collect ====
void collect(int bnum)
{
    BRole[bnum].Progress += 5;
}

// ==== Rest ====
void Rest(int bnum)
{
    int rnum = BRole[bnum].rnum;
    if (rnum < 0) return;
    RRole[rnum].PhyPower += 5 + rand() % 5;
    if (RRole[rnum].PhyPower > MAX_PHYSICAL_POWER)
        RRole[rnum].PhyPower = MAX_PHYSICAL_POWER;
    BRole[bnum].Acted = 1;
    BRole[bnum].Progress -= 240;
}

// ==== TODO stub implementations ====

int  SelectTeamMembers() { return 0x3F; /* TODO */ }
void ShowMultiMenu(int max, int menu, int status) { /* TODO */ }
void BattleMainControl() { /* TODO */ }
void OldBattleMainControl() { /* TODO */ }
int  CountProgress() { return -1; /* TODO */ }
void ReArrangeBRole() { /* TODO */ }
int  BattleMenu(int bnum) { return 0; /* TODO */ }
void ShowBMenu(int MenuStatus, int menu, int max) { /* TODO */ }
void ShowBMenu2(int MenuStatus, int menu, int max, int bnum) { /* TODO */ }
void MoveRole(int bnum) { /* TODO */ }
void MoveAmination(int bnum) { /* TODO */ }
bool Selectshowstatus(int bnum) { return false; /* TODO */ }
bool SelectAim(int bnum, int step) { return false; /* TODO */ }
bool SelectRange(int bnum, int AttAreaType, int step, int range) { return false; /* TODO */ }
bool SelectDirector(int bnum, int AttAreaType, int step, int range) { return false; /* TODO */ }
bool SelectCross(int bnum, int AttAreaType, int step, int range) { return false; /* TODO */ }
bool SelectFar(int bnum, int mnum, int level) { return false; /* TODO */ }
bool SelectLine(int bnum, int AttAreaType, int step, int range) { return false; /* TODO */ }
void Attack(int bnum) { /* TODO */ }
void AttackAction(int bnum, int mnum, int level) { /* TODO */ }
void ShowMagicName(int mnum) { /* TODO */ }
int  SelectMagic(int bnum) { return -1; /* TODO */ }
void ShowMagicMenu(int bnum, int menustatus, int menu, int max) { /* TODO */ }
void SetAminationPosition(int mode, int step, int range) { /* TODO */ }
void PlayMagicAmination(int bnum, int bigami, int enum_, int level) { /* TODO */ }
void PlayActionAmination(int bnum, int mode) { /* TODO */ }
void CalHurtRole(int bnum, int mnum, int level) { /* TODO */ }
int  CalHurtValue(int bnum1, int bnum2, int mnum, int level) { return 0; /* TODO */ }
void ShowSimpleStatus(int rnum, int x, int y) { /* TODO */ }
void showprogress() { /* TODO */ }
void BattleMenuItem(int bnum) { /* TODO */ }
void UsePoision(int bnum) { /* TODO */ }
void Medcine(int bnum) { /* TODO */ }
void MedFrozen(int bnum) { /* TODO */ }
void MedPoision(int bnum) { /* TODO */ }
void UseHiddenWeapen(int bnum, int inum) { /* TODO */ }
void AutoBattle(int bnum) { /* TODO */ }
void AutoUseItem(int bnum, int list) { /* TODO */ }
void PetEffect() { /* TODO */ }
void AutoBattle2(int bnum) { /* TODO */ }
void Auto(int bnum) { /* TODO */ }

void trymoveattack(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int bnum, int mnum, int level) { /* TODO */ }
void calline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void calarea(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void calNewline(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void calpoint(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void calcross(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void caldirdiamond(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void caldirangle(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void calfar(int& Mx1, int& My1, int& Ax1, int& Ay1, int& tempmaxhurt,
    int curX, int curY, int bnum, int mnum, int level) { /* TODO */ }
void nearestmove(int& Mx, int& My, int bnum) { /* TODO */ }
void farthestmove(int& Mx, int& My, int bnum) { /* TODO */ }
void trymovecure(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum) { /* TODO */ }
void cureaction(int bnum) { /* TODO */ }
void trymoveHidden(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum, int inum) { /* TODO */ }
void Hiddenaction(int bnum, int inum) { /* TODO */ }
void trymoveUsePoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum) { /* TODO */ }
void UsePoiaction(int bnum) { /* TODO */ }
void trymoveMedPoi(int& Mx1, int& My1, int& Ax1, int& Ay1, int bnum) { /* TODO */ }
void MedPoiaction(int bnum) { /* TODO */ }
void ShowModeMenu(int menu) { /* TODO */ }
int  SelectAutoMode() { return 0; /* TODO */ }
void ShowTeamModeMenu(int menu) { /* TODO */ }
bool TeamModeMenu() { return false; /* TODO */ }