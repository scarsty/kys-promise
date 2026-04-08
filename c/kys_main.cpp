// kys_main.cpp - 主程序实现
// 对应 kys_main.pas 的 implementation 段

#include "kys_main.h"
#include "kys_engine.h"
#include "kys_event.h"
#include "kys_battle.h"
#include "kys_littlegame.h"
#include "kys_gfx.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---- 模块内部辅助 ----
// Pascal IniFile 简易替代
static std::string IniReadString(const std::string& file, const std::string& section, const std::string& key, const std::string& def)
{
    // 简单INI读取
    std::ifstream f(file);
    if (!f) return def;
    std::string line, curSec;
    while (std::getline(f, line))
    {
        if (line.empty()) continue;
        if (line[0] == '[')
        {
            auto end = line.find(']');
            if (end != std::string::npos)
                curSec = line.substr(1, end - 1);
            continue;
        }
        if (_stricmp(curSec.c_str(), section.c_str()) != 0) continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        // trim
        while (!k.empty() && k.back() == ' ') k.pop_back();
        if (_stricmp(k.c_str(), key.c_str()) != 0) continue;
        std::string v = line.substr(eq + 1);
        while (!v.empty() && v.front() == ' ') v.erase(v.begin());
        while (!v.empty() && (v.back() == '\r' || v.back() == '\n')) v.pop_back();
        return v;
    }
    return def;
}

static int IniReadInt(const std::string& file, const std::string& section, const std::string& key, int def)
{
    std::string s = IniReadString(file, section, key, "");
    if (s.empty()) return def;
    try { return std::stoi(s); }
    catch (...) { return def; }
}

static void IniWriteInt(const std::string& file, const std::string& section, const std::string& key, int value)
{
    // TODO: 实现INI写入
}

static std::string iniFilePath;

// 文件读写辅助 (对应 Pascal 的 fileopen/fileread/filewrite/fileseek/filecreate/fileclose)
static FILE* PasFileOpen(const std::string& path, const char* mode)
{
    FILE* f = nullptr;
    fopen_s(&f, path.c_str(), mode);
    return f;
}

static int PasFileRead(FILE* f, void* buf, int size)
{
    if (!f) return 0;
    return static_cast<int>(fread(buf, 1, size, f));
}

static int PasFileWrite(FILE* f, const void* buf, int size)
{
    if (!f) return 0;
    return static_cast<int>(fwrite(buf, 1, size, f));
}

static int PasFileSeek(FILE* f, int offset, int origin)
{
    fseek(f, offset, origin);
    return static_cast<int>(ftell(f));
}

static void PasFileClose(FILE* f)
{
    if (f) fclose(f);
}

// ==== Run - 主入口 ====

int Run(int argc, char* argv[])
{
    AppPath = "";
#ifdef _WIN32
    // Windows: 使用当前目录
#else
    if (argc > 0)
    {
        fs::path p(argv[0]);
        AppPath = p.parent_path().string() + "/";
    }
#endif

    ReadFiles();

    // 初始化字体
    TTF_Init();
    Font = TTF_OpenFont((AppPath + CHINESE_FONT).c_str(), CHINESE_FONT_SIZE);
    EngFont = TTF_OpenFont((AppPath + ENGLISH_FONT).c_str(), ENGLISH_FONT_SIZE);
    if (Font == nullptr) return 1;

    // 初始化音频
    MIX_Init();
    InitialMusic();

    // 初始化视频
    srand(static_cast<unsigned>(time(nullptr)));
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Quit();
        return 1;
    }

    std::string title = "The Story Before That Legend";
    ScreenFlag = SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow(title.c_str(), RESOLUTIONX, RESOLUTIONY, ScreenFlag);

    SDL_GetWindowSize(window, &RESOLUTIONX, &RESOLUTIONY);
    kyslog(std::format("Resolution: {} {}", RESOLUTIONX, RESOLUTIONY));
    if (CellPhone == 1)
    {
        if (RESOLUTIONY > RESOLUTIONX)
        {
            ScreenRotate = 0;
            std::swap(RESOLUTIONX, RESOLUTIONY);
        }
    }

    render = SDL_CreateRenderer(window, "direct3d");
    screen = SDL_CreateSurface(CENTER_X * 2, CENTER_Y * 2,
        SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
    screenTex = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, CENTER_X * 2, CENTER_Y * 2);
    freshscreen = SDL_CreateSurface(CENTER_X * 2, CENTER_Y * 2,
        SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));

    SDL_AddEventWatch(reinterpret_cast<SDL_EventFilter>(EventFilter), nullptr);

    if (CellPhone == 1)
        SDL_SetWindowFullscreen(window, true);

    Start();

    TTF_CloseFont(Font);
    TTF_CloseFont(EngFont);
    TTF_Quit();
    MIX_Quit();
    SDL_Quit();
    return 0;
}

void Quit()
{
    TTF_CloseFont(Font);
    TTF_CloseFont(EngFont);
    TTF_Quit();
    MIX_Quit();
    SDL_Quit();
    exit(1);
}

// ==== StartAmi - 开头字幕 ====

void StartAmi()
{
    instruct_14();
    DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 100);
    ShowTitle(4545, 28515);
    DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 100);
    ShowTitle(4546, 28515);
    instruct_14();
}

// ==== ReadFiles - 读取必须的文件 ====

void ReadFiles()
{
    versionstr = "Promise";
    // TODO: 读取文件版本信息

    std::string filename = AppPath + "kysmod.ini";
    iniFilePath = filename;

    BEGIN_BATTLE_ROLE_PIC = 1;
    BEGIN_EVENT = 101;
    BEGIN_Scene = 0;
    BEGIN_Sx = 40;
    BEGIN_Sy = 38;
    MAX_PHYSICAL_POWER = 100;
    BEGIN_WALKPIC = 2500;
    MONEY_ID = 0;
    COMPASS_ID = 1;
    MAX_LEVEL = 30;
    MAX_WEAPON_MATCH = 7;
    MIN_KNOWLEDGE = 0;
    MAX_HP = 9999;
    MAX_MP = 9999;
    LIFE_HURT = 10;
    MAP_ID = 303;
    MusicVolume = IniReadInt(filename, "constant", "MUSIC_VOLUME", 64);
    SoundVolume = IniReadInt(filename, "constant", "SOUND_VOLUME", 32);
    MAX_ITEM_AMOUNT = 400;
    GameSpeed = std::max(1, IniReadInt(filename, "constant", "GAME_SPEED", 10));
    Simple = IniReadInt(filename, "Set", "simple", 0);
    Showanimation = IniReadInt(filename, "Set", "animation", 0);
    fullscreen = IniReadInt(filename, "Set", "fullscreen", 0);
    BattleMode = IniReadInt(filename, "Set", "BattleMode", 0);
    HW = IniReadInt(filename, "Set", "HW", 0);
    SMOOTH = IniReadInt(filename, "set", "SMOOTH", 1);
    GLHR = IniReadInt(filename, "set", "GLHR", 1);
    RESOLUTIONX = IniReadInt(filename, "Set", "RESOLUTIONX", CENTER_X * 2);
    RESOLUTIONY = IniReadInt(filename, "set", "RESOLUTIONY", CENTER_Y * 2);
    std::string dbgstr = IniReadString(filename, "Set", "debug", "0");
    Debug = 0;

    MaxProList[15] = 1; // [58] -> index 15

    ITEM_PIC.resize(MAX_ITEM_AMOUNT);

    if (CellPhone != 0)
    {
        ShowVirtualKey = IniReadInt(filename, "system", "Virtual_Key", 1);
        VirtualKeyX = IniReadInt(filename, "system", "Virtual_Key_X", 80);
        VirtualKeyY = IniReadInt(filename, "system", "Virtual_Key_Y", 250);
        if (fs::exists(AppPath + "resource/u.png"))
        {
            VirtualKeyU = IMG_Load((AppPath + "resource/u.png").c_str());
            VirtualKeyD = IMG_Load((AppPath + "resource/d.png").c_str());
            VirtualKeyL = IMG_Load((AppPath + "resource/l.png").c_str());
            VirtualKeyR = IMG_Load((AppPath + "resource/r.png").c_str());
            VirtualKeyA = IMG_Load((AppPath + "resource/a.png").c_str());
            VirtualKeyB = IMG_Load((AppPath + "resource/b.png").c_str());
        }
        else
            ShowVirtualKey = 0;
    }
    else
        ShowVirtualKey = 0;

    // 读取调色板
    if (fs::exists(AppPath + "resource/pallet.COL"))
    {
        FILE* c = PasFileOpen(AppPath + "resource/pallet.COL", "rb");
        PasFileRead(c, &Col[0][0], 4 * 768);
        PasFileClose(c);
    }
    resetpallet();

    // 读取主地图贴图索引
    auto loadIdxGrp = [](const std::string& idxPath, const std::string& grpPath,
        std::vector<int>& idx, std::vector<uint8_t>& pic)
    {
        FILE* fi = PasFileOpen(idxPath, "rb");
        FILE* fg = PasFileOpen(grpPath, "rb");
        if (!fi || !fg) { PasFileClose(fi); PasFileClose(fg); return; }
        int len = PasFileSeek(fg, 0, SEEK_END);
        PasFileSeek(fg, 0, SEEK_SET);
        pic.resize(len);
        PasFileRead(fg, pic.data(), len);
        int tnum = PasFileSeek(fi, 0, SEEK_END) / 4;
        PasFileSeek(fi, 0, SEEK_SET);
        idx.resize(tnum);
        PasFileRead(fi, idx.data(), tnum * 4);
        PasFileClose(fg);
        PasFileClose(fi);
    };

    loadIdxGrp(AppPath + "resource/mmap.idx", AppPath + "resource/mmap.grp", MIdx, MPic);
    loadIdxGrp(AppPath + "resource/sdx", AppPath + "resource/smp", SIdx, SPic);
    loadIdxGrp(AppPath + "resource/wdx", AppPath + "resource/wmp", WIdx, WPic);

    // Cloud
    {
        FILE* fi = PasFileOpen(AppPath + "resource/cloud.idx", "rb");
        FILE* fg = PasFileOpen(AppPath + "resource/cloud.grp", "rb");
        if (fi && fg)
        {
            int len = PasFileSeek(fg, 0, SEEK_END);
            PasFileSeek(fg, 0, SEEK_SET);
            PasFileRead(fg, CPic, len);
            int tnum = PasFileSeek(fi, 0, SEEK_END) / 4;
            PasFileSeek(fi, 0, SEEK_SET);
            PasFileRead(fi, CIdx, tnum * 4);
        }
        PasFileClose(fi);
        PasFileClose(fg);
    }

    // Scene Pic
    if (fs::exists(AppPath + Scene_file))
    {
        FILE* grp = PasFileOpen(AppPath + Scene_file, "rb");
        int len = 0;
        PasFileRead(grp, &len, 4);
        ScenePic.resize(len);
        for (int i = 0; i < len; i++)
            ScenePic[i] = GetPngPic(grp, i);
        PasFileClose(grp);
    }

    // Head Pic
    if (fs::exists(AppPath + HEADS_file))
    {
        FILE* grp = PasFileOpen(AppPath + HEADS_file, "rb");
        int len = 0;
        PasFileRead(grp, &len, 4);
        Head_Pic.resize(len);
        for (int i = 0; i < len; i++)
            Head_Pic[i] = GetPngPic(grp, i);
        PasFileClose(grp);
    }

    // Skill Pic
    if (fs::exists(AppPath + Skill_file))
    {
        FILE* grp = PasFileOpen(AppPath + Skill_file, "rb");
        int len = 0;
        PasFileRead(grp, &len, 4);
        SkillPIC.resize(len);
        for (int i = 0; i < len; i++)
            SkillPIC[i] = GetPngPic(grp, i);
        PasFileClose(grp);
    }

    // Background Pic
    if (fs::exists(AppPath + BackGround_file))
    {
        FILE* grp = PasFileOpen(AppPath + BackGround_file, "rb");
        BEGIN_PIC = GetPngPic(grp, 0);
        MAGIC_PIC = GetPngPic(grp, 1);
        STATE_PIC = GetPngPic(grp, 2);
        SYSTEM_PIC = GetPngPic(grp, 3);
        MAP_PIC = GetPngPic(grp, 4);
        SKILL_PIC = GetPngPic(grp, 5);
        MENUESC_PIC = GetPngPic(grp, 6);
        MenuescBack_PIC = GetPngPic(grp, 7);
        battlepic = GetPngPic(grp, 8);
        TEAMMATE_PIC = GetPngPic(grp, 9);
        MENUITEM_PIC = GetPngPic(grp, 10);
        PROGRESS_PIC = GetPngPic(grp, 11);
        MATESIGN_PIC = GetPngPic(grp, 12);
        ENEMYSIGN_PIC = GetPngPic(grp, 13);
        SELECTEDENEMY_PIC = GetPngPic(grp, 14);
        SELECTEDMATE_PIC = GetPngPic(grp, 15);
        NowPROGRESS_PIC = GetPngPic(grp, 16);
        angryprogress_pic = GetPngPic(grp, 17);
        angrycollect_pic = GetPngPic(grp, 18);
        angryfull_pic = GetPngPic(grp, 19);
        DEATH_PIC = GetPngPic(grp, 20);
        Maker_Pic = GetPngPic(grp, 21);
        PasFileClose(grp);
    }

    // 读取主地图数据
    auto loadBin = [](const std::string& path, void* buf, int size)
    {
        FILE* c = PasFileOpen(path, "rb");
        if (c) { PasFileRead(c, buf, size); PasFileClose(c); }
    };

    loadBin(AppPath + "resource/earth.002", Earth, sizeof(Earth));
    loadBin(AppPath + "resource/surface.002", Surface, sizeof(Surface));
    loadBin(AppPath + "resource/building.002", Building, sizeof(Building));
    loadBin(AppPath + "resource/buildx.002", BuildX, sizeof(BuildX));
    loadBin(AppPath + "resource/buildy.002", BuildY, sizeof(BuildY));
    loadBin(AppPath + "list/Set.bin", SetNum, sizeof(SetNum));
    loadBin(AppPath + "list/levelup.bin", LevelUpList, 200);
}

// ==== GetPngPic (FILE* overload) ====
// Pascal 中 GetPngPic(grp: integer, num: integer) 中 grp 是文件句柄
// 这里用 FILE* 重载

TPic GetPngPic(FILE* f, int num)
{
    TPic result{};
    if (!f) return result;

    int32_t offset = 0, len = 0;
    // 每个 PNG 条目: 4字节偏移 + 4字节长度 + 数据
    // 实际格式: 先总数(4字节), 然后每个条目:
    //   4字节x, 4字节y, 4字节black, 4字节大小, 数据
    int32_t x = 0, y = 0, black = 0, size = 0;
    PasFileRead(f, &x, 4);
    PasFileRead(f, &y, 4);
    PasFileRead(f, &black, 4);
    PasFileRead(f, &size, 4);
    result.x = x;
    result.y = y;
    result.black = black;
    if (size > 0)
    {
        std::vector<uint8_t> buf(size);
        PasFileRead(f, buf.data(), size);
        result.pic = ReadPicFromByte(buf.data(), size);
    }
    return result;
}

// ==== Start - 开头画面 ====

void Start()
{
    StopMP3();
    PlayMP3(106, -1);
    int ingame = 0;
    PlayBeginningMovie(0, -1);
    display_imgFromSurface(BEGIN_PIC, 0, 0);
    DrawGBKShadowText(versionstr.c_str(), 5, CENTER_Y * 2 - 30, ColColor(0x64), ColColor(0x66));
    MStep = 0;
    Where = 3;
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    int menu = 0;
    RItemList.resize(MAX_ITEM_AMOUNT);
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        RItemList[i].Number = -1;
        RItemList[i].Amount = 0;
    }

    Cloud.resize(CLOUD_AMOUNT);
    for (int i = 0; i < CLOUD_AMOUNT; i++)
        CloudCreate(i);

    int x = 275, y = 290;
    DrawTitlePic(0, x, y);
    DrawTitlePic(menu + 1, x, y + menu * 20);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    while (ingame == 0)
    {
        while (SDL_WaitEvent(&event))
        {
            CheckBasicEvent();
            int xm, ym;
            // 选择第2项退出
            if (((event.type == SDL_EVENT_KEY_UP) && (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)) ||
                ((event.type == SDL_EVENT_MOUSE_BUTTON_UP) && (event.button.button == SDL_BUTTON_LEFT))) 
            {
                if (menu == 2)
                {
                    ingame = 1;
                    Quit();
                }
                // 选择第0项新游戏
                if (menu == 0)
                {
                    if (InitialRole())
                    {
                        ShowMR = false;
                        CurScene = BEGIN_Scene;
                        StopMP3();
                        PlayMP3(RScene[CurScene].ExitMusic, -1);
                        WalkInScene(1);
                        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                        break;
                    }
                    Redraw();
                    DrawTitlePic(0, x, y);
                    DrawTitlePic(menu + 1, x, y + menu * 20);
                    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                }
                SDL_GetMouseState2(xm, ym);
                // 选择第1项读取
                if (menu == 1)
                {
                    ShowMR = true;
                    if (MenuLoadAtBeginning())
                    {
                        PlayBeginningMovie(26, 0);
                        instruct_14();
                        event.key.key = 0;
                        CurEvent = -1;
                        break;
                    }
                    else
                    {
                        DrawTitlePic(0, x, y);
                        DrawTitlePic(menu + 1, x, y + menu * 20);
                        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                    }
                }
            }
            if ((event.type == SDL_EVENT_KEY_DOWN) && (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8))
            {
                menu--;
                if (menu < 0) menu = 2;
                DrawTitlePic(0, x, y);
                DrawTitlePic(menu + 1, x, y + menu * 20);
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if ((event.type == SDL_EVENT_KEY_DOWN) && (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2))
            {
                menu++;
                if (menu > 2) menu = 0;
                DrawTitlePic(0, x, y);
                DrawTitlePic(menu + 1, x, y + menu * 20);
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                SDL_GetMouseState2(xm, ym);
                if (xm > x && xm < x + 80 && ym > y && ym < y + 60)
                {
                    int menup = menu;
                    menu = (ym - y) / 20;
                    if (menu != menup)
                    {
                        DrawTitlePic(0, x, y);
                        DrawTitlePic(menu + 1, x, y + menu * 20);
                        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                    }
                }
                else if (CellPhone == 0)
                    menu = -1;
            }
        }
        event.key.key = 0;
        event.button.button = 0;
        if (Where == 1) WalkInScene(0);
        Walk();
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
}

// ==== XorCount ====

void XorCount(uint8_t* Data, uint8_t xornum, int length)
{
    for (int i = 0; i < length; i++)
    {
        *Data = static_cast<uint8_t>(*Data ^ xornum);
        Data++;
    }
}

// ==== LoadR - 读入存档 ====

void LoadR(int num)
{
    SaveNum = num;
    std::string filename = "R" + std::to_string(num);
    if (num == 0) filename = "ranger";

    FILE* idx = PasFileOpen(AppPath + "save/ranger.idx", "rb");
    FILE* grp = PasFileOpen(AppPath + "save/" + filename + ".grp", "rb");
    if (!idx || !grp) { PasFileClose(idx); PasFileClose(grp); return; }

    int RoleOffset, ItemOffset, SceneOffset, MagicOffset, WeiShopOffset, len;
    PasFileRead(idx, &RoleOffset, 4);
    PasFileRead(idx, &ItemOffset, 4);
    PasFileRead(idx, &SceneOffset, 4);
    PasFileRead(idx, &MagicOffset, 4);
    PasFileRead(idx, &WeiShopOffset, 4);
    PasFileRead(idx, &len, 4);

    PasFileSeek(grp, 0, SEEK_SET);
    PasFileRead(grp, &InShip, 2);
    PasFileRead(grp, &Where, 2);
    PasFileRead(grp, &My, 2);
    PasFileRead(grp, &Mx, 2);
    PasFileRead(grp, &Sy, 2);
    PasFileRead(grp, &Sx, 2);
    PasFileRead(grp, &MFace, 2);
    PasFileRead(grp, &ShipX, 2);
    PasFileRead(grp, &ShipY, 2);
    PasFileRead(grp, &time_, 2);
    PasFileRead(grp, &timeevent, 2);
    PasFileRead(grp, &RandomEvent, 2);
    PasFileRead(grp, &SFace, 2);
    PasFileRead(grp, &ShipFace, 2);
    PasFileRead(grp, &gametime, 2);
    PasFileRead(grp, TeamList, 2 * 6);
    PasFileRead(grp, RItemList.data(), sizeof(TItemList) * MAX_ITEM_AMOUNT);

    RRole.resize((ItemOffset - RoleOffset) / sizeof(TRole));
    PasFileRead(grp, RRole.data(), ItemOffset - RoleOffset);

    RItem.resize((SceneOffset - ItemOffset) / sizeof(TItem));
    PasFileRead(grp, RItem.data(), SceneOffset - ItemOffset);

    RScene.resize((MagicOffset - SceneOffset) / sizeof(TScene));
    PasFileRead(grp, RScene.data(), MagicOffset - SceneOffset);

    RMagic.resize((WeiShopOffset - MagicOffset) / sizeof(TMagic));
    PasFileRead(grp, RMagic.data(), WeiShopOffset - MagicOffset);

    RShop.resize((len - WeiShopOffset) / sizeof(TShop));
    PasFileRead(grp, RShop.data(), len - WeiShopOffset);

    PasFileClose(idx);
    PasFileClose(grp);

    if (static_cast<int16_t>(Where) < 0)
        Where = 0;
    else
    {
        CurScene = Where;
        Where = 1;
    }

    ReSetEntrance();
    int sceneCount = static_cast<int>(RScene.size());
    SData.resize(sceneCount);
    for (auto& s : SData) s.resize(6);
    DData.resize(sceneCount);
    for (auto& d : DData) d.resize(200);

    filename = "S" + std::to_string(num);
    if (num == 0) filename = "Allsin";
    grp = PasFileOpen(AppPath + "save/" + filename + ".grp", "rb");
    if (grp)
    {
        for (int i = 0; i < sceneCount; i++)
            for (int layer = 0; layer < 6; layer++)
                PasFileRead(grp, &SData[i][layer][0][0], 64 * 64 * 2);
        PasFileClose(grp);
    }

    filename = "D" + std::to_string(num);
    if (num == 0) filename = "Alldef";
    grp = PasFileOpen(AppPath + "save/" + filename + ".grp", "rb");
    if (grp)
    {
        for (int i = 0; i < sceneCount; i++)
            for (int e = 0; e < 200; e++)
                PasFileRead(grp, &DData[i][e][0], 11 * 2);
        PasFileClose(grp);
    }

    if (BattleMode > std::min(static_cast<int>(gametime), 2))
        BattleMode = std::min(static_cast<int>(gametime), 2);
    MAX_LEVEL = std::min(50, 30 + gametime * 10);
}

// ==== SaveR - 存档 ====

void SaveR(int num)
{
    SaveNum = num;
    std::string filename = "R" + std::to_string(num);
    if (num == 0) filename = "ranger";

    FILE* ridx = PasFileOpen(AppPath + "save/ranger.idx", "rb");
    FILE* rgrp = PasFileOpen(AppPath + "save/" + filename + ".grp", "wb");
    if (!ridx || !rgrp) { PasFileClose(ridx); PasFileClose(rgrp); return; }

    int RoleOffset, ItemOffset, SceneOffset, MagicOffset, WeiShopOffset, len;
    PasFileRead(ridx, &RoleOffset, 4);
    PasFileRead(ridx, &ItemOffset, 4);
    PasFileRead(ridx, &SceneOffset, 4);
    PasFileRead(ridx, &MagicOffset, 4);
    PasFileRead(ridx, &WeiShopOffset, 4);
    PasFileRead(ridx, &len, 4);

    PasFileSeek(rgrp, 0, SEEK_SET);
    PasFileWrite(rgrp, &InShip, 2);
    if (Where == 0)
    {
        int16_t neg = -1;
        PasFileWrite(rgrp, &neg, 2);
    }
    else
        PasFileWrite(rgrp, &CurScene, 2);
    PasFileWrite(rgrp, &My, 2);
    PasFileWrite(rgrp, &Mx, 2);
    PasFileWrite(rgrp, &Sy, 2);
    PasFileWrite(rgrp, &Sx, 2);
    PasFileWrite(rgrp, &MFace, 2);
    PasFileWrite(rgrp, &ShipX, 2);
    PasFileWrite(rgrp, &ShipY, 2);
    PasFileWrite(rgrp, &time_, 2);
    PasFileWrite(rgrp, &timeevent, 2);
    PasFileWrite(rgrp, &RandomEvent, 2);
    PasFileWrite(rgrp, &SFace, 2);
    PasFileWrite(rgrp, &ShipFace, 2);
    PasFileWrite(rgrp, &gametime, 2);
    PasFileWrite(rgrp, TeamList, 2 * 6);
    PasFileWrite(rgrp, RItemList.data(), sizeof(TItemList) * MAX_ITEM_AMOUNT);

    PasFileWrite(rgrp, RRole.data(), ItemOffset - RoleOffset);
    PasFileWrite(rgrp, RItem.data(), SceneOffset - ItemOffset);
    PasFileWrite(rgrp, RScene.data(), MagicOffset - SceneOffset);
    PasFileWrite(rgrp, RMagic.data(), WeiShopOffset - MagicOffset);
    PasFileWrite(rgrp, RShop.data(), len - WeiShopOffset);

    int SceneAmount = static_cast<int>(RScene.size());

    filename = "S" + std::to_string(num);
    if (num == 0) filename = "Allsin";
    FILE* sgrp = PasFileOpen(AppPath + "save/" + filename + ".grp", "wb");
    if (sgrp)
    {
        for (int i = 0; i < SceneAmount; i++)
            for (int layer = 0; layer < 6; layer++)
                PasFileWrite(sgrp, &SData[i][layer][0][0], 64 * 64 * 2);
        PasFileClose(sgrp);
    }

    filename = "D" + std::to_string(num);
    if (num == 0) filename = "Alldef";
    FILE* dgrp = PasFileOpen(AppPath + "save/" + filename + ".grp", "wb");
    if (dgrp)
    {
        for (int i = 0; i < SceneAmount; i++)
            for (int e = 0; e < 200; e++)
                PasFileWrite(dgrp, &DData[i][e][0], 11 * 2);
        PasFileClose(dgrp);
    }

    PasFileClose(rgrp);
    PasFileClose(ridx);
}

// ==== WaitAnyKey ====

int WaitAnyKey()
{
    event.key.key = 0;
    event.button.button = 0;
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            if (event.key.key != 0 || event.button.button != 0)
                break;
    }
    int result = event.key.key;
    event.key.key = 0;
    event.button.button = 0;
    return result;
}

void WaitAnyKey(int16_t* keycode, int16_t* x, int16_t* y)
{
    event.key.key = 0;
    event.button.button = 0;
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP && event.key.key != 0)
        {
            *keycode = static_cast<int16_t>(event.key.key);
            break;
        }
        if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button != 0)
        {
            int xm, ym;
            SDL_GetMouseState2(xm, ym);
            *keycode = -1;
            *x = static_cast<int16_t>(xm);
            *y = static_cast<int16_t>(ym + 30);
            break;
        }
    }
    event.key.key = 0;
    event.button.button = 0;
}

// ==== Walk - 主地图行走 ====

void Walk()
{
    Where = 0;
    uint32 next_time = SDL_GetTicks();
    uint32 next_time2 = SDL_GetTicks();
    uint32 next_time3 = SDL_GetTicks();

    int walking = 0;
    resetpallet();
    DrawMMap();
    StopMP3();
    PlayMP3(16, -1);
    Still = 0;
    int speed = 0;
    int stillcount = 0;

    event.key.key = 0;
    while (SDL_PollEvent(&event) || true)
    {
        int needrefresh = 0;
        if (Where >= 3) break;

        uint32 now = SDL_GetTicks();

        if (static_cast<int>(now - next_time2) > 0)
        {
            ChangeCol();
            next_time2 = now + 200;
        }

        if (static_cast<int>(now - next_time3) > 0)
        {
            for (int i = 0; i < CLOUD_AMOUNT; i++)
            {
                Cloud[i].Positionx += Cloud[i].Speedx;
                Cloud[i].Positiony += Cloud[i].Speedy;
                if (Cloud[i].Positionx > 17279 || Cloud[i].Positionx < 0 ||
                    Cloud[i].Positiony > 8639 || Cloud[i].Positiony < 0)
                    CloudCreateOnSide(i);
            }
            next_time3 = now + 40;
        }

        if (static_cast<int>(now - next_time) > 0 && Where == 0)
        {
            if (walking == 0) stillcount++;
            else stillcount = 0;
            if (stillcount >= 10)
            {
                Still = 1;
                MStep++;
                if (MStep > 6) MStep = 1;
            }
            next_time = now + 300;
        }

        CheckBasicEvent();

        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { MFace = 2; walking = 2; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { MFace = 1; walking = 2; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { MFace = 0; walking = 2; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { MFace = 3; walking = 2; }
            break;
        case SDL_EVENT_KEY_UP:
            walking = 0;
            speed = 0;
            if (event.key.key == SDLK_ESCAPE)
            {
                NewMenuEsc();
                nowstep = -1;
                walking = 0;
            }
            break;
        }

        if (walking > 0)
        {
            int dx = 0, dy = 0;
            switch (MFace)
            {
            case 0: dy = -1; break;
            case 1: dx = 1; break;
            case 2: dx = -1; break;
            case 3: dy = 1; break;
            }
            if (CanWalk(Mx + dx, My + dy))
            {
                Mx += dx;
                My += dy;
                MStep++;
                if (MStep > 6) MStep = 1;
                Still = 0;
                needrefresh = 1;
            }
        }

        if (needrefresh || true) // 始终刷新
        {
            DrawMMap();
            DrawClouds();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }

        SDL_Delay(static_cast<uint32>(GameSpeed));
    }
}

// ==== CanWalk ====

bool CanWalk(int x, int y)
{
    if (x < 0 || x >= 480 || y < 0 || y >= 480) return false;
    if (Building[x][y] > 0) return false;
    if (Entrance[x][y] > 0) return true;
    if (Earth[x][y] > 0) return true;
    return false;
}

// ==== CheckEntrance ====

bool CheckEntrance()
{
    if (Entrance[Mx][My] > 0)
    {
        int snum = Entrance[Mx][My];
        if (snum > 0 && snum < static_cast<int>(RScene.size()))
        {
            CurScene = snum;
            return true;
        }
    }
    return false;
}

// ==== ReSetEntrance ====

void ReSetEntrance()
{
    std::memset(Entrance, 0, sizeof(Entrance));
    for (int i = 0; i < static_cast<int>(RScene.size()); i++)
    {
        if (RScene[i].MainEntranceX1 >= 0 && RScene[i].MainEntranceX1 < 480 &&
            RScene[i].MainEntranceY1 >= 0 && RScene[i].MainEntranceY1 < 480)
        {
            for (int x = RScene[i].MainEntranceX1; x <= RScene[i].MainEntranceX2 && x < 480; x++)
                for (int y = RScene[i].MainEntranceY1; y <= RScene[i].MainEntranceY2 && y < 480; y++)
                    Entrance[x][y] = i;
        }
    }
}

// ==== ShowRandomAttribute ====

void ShowRandomAttribute(bool ran)
{
    std::wstring str = L" \u8CC7\u8CEA"; // 資質
    std::wstring tip = L" \u9078\u5B9A\u5C6C\u6027\u5F8C\u6309\u56DE\u8ECA\uFF0CEsc\u8FD4\u56DE";
    if (ran)
    {
        RRole[0].MaxHP = 51 + rand() % 50;
        RRole[0].CurrentHP = RRole[0].MaxHP;
        RRole[0].MaxMP = 51 + rand() % 50;
        RRole[0].CurrentMP = RRole[0].MaxMP;
        RRole[0].MPType = rand() % 2;
        RRole[0].IncLife = 1 + rand() % 10;

        RRole[0].Attack = 25 + rand() % 6;
        RRole[0].Speed = 25 + rand() % 6;
        RRole[0].Defence = 25 + rand() % 6;
        RRole[0].Medcine = 25 + rand() % 6;
        RRole[0].UsePoi = 25 + rand() % 6;
        RRole[0].MedPoi = 25 + rand() % 6;
        RRole[0].Fist = 25 + rand() % 6;
        RRole[0].Sword = 25 + rand() % 6;
        RRole[0].Knife = 25 + rand() % 6;
        RRole[0].Unusual = 25 + rand() % 6;
        RRole[0].HidWeapon = 25 + rand() % 6;
        RRole[0].Aptitude = 1 + rand() % 100;
    }
    Redraw();
    ShowStatus(0);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 30, CENTER_Y + 111, ColColor(0x21), ColColor(0x23));
    std::wstring str0 = std::format(L"{:4d}", RRole[0].Aptitude);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str0.data()), 150, CENTER_Y + 111, ColColor(0x63), ColColor(0x66));
    DrawShadowText(reinterpret_cast<uint16_t*>(tip.data()), 210, CENTER_Y + 111, ColColor(0x5), ColColor(0x7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

bool RandomAttribute()
{
    int keyvalue;
    do {
        ShowRandomAttribute(true);
        keyvalue = WaitAnyKey();
    } while (keyvalue != SDLK_Y && keyvalue != SDLK_ESCAPE && keyvalue != SDLK_RETURN);
    if (keyvalue == SDLK_Y || keyvalue == SDLK_RETURN)
    {
        ShowRandomAttribute(false);
        return true;
    }
    return false;
}

// ==== CloudCreate / CloudCreateOnSide ====

void CloudCreate(int num)
{
    if (num < 0 || num >= static_cast<int>(Cloud.size())) return;
    Cloud[num].Positionx = rand() % 17280;
    Cloud[num].Positiony = rand() % 8640;
    Cloud[num].Speedx = 1 + rand() % 3;
    Cloud[num].Speedy = 0;
    Cloud[num].Picnum = rand() % 20;
    Cloud[num].Shadow = 0;
    Cloud[num].Alpha = 20 + rand() % 30;
    Cloud[num].MixColor = 0;
    Cloud[num].MixAlpha = 0;
}

void CloudCreateOnSide(int num)
{
    if (num < 0 || num >= static_cast<int>(Cloud.size())) return;
    Cloud[num].Positiony = rand() % 8640;
    Cloud[num].Speedx = 1 + rand() % 3;
    Cloud[num].Speedy = 0;
    Cloud[num].Picnum = rand() % 20;
    Cloud[num].Shadow = 0;
    Cloud[num].Alpha = 20 + rand() % 30;
    if (Cloud[num].Positionx > 17279)
        Cloud[num].Positionx = 0;
    else
        Cloud[num].Positionx = 17279;
}

// ==== FileExistsUTF8 ====

bool FileExistsUTF8(const char* filename)
{
    return fs::exists(filename);
}

bool FileExistsUTF8(const std::string& filename)
{
    return fs::exists(filename);
}

// ==== 以下函数为桩实现，需后续逐一完善 ====

bool InitialRole() { /* TODO */ return false; }
int WalkInScene(int Open) { /* TODO */ return 0; }
void ShowSceneName(int snum) { /* TODO */ }
bool CanWalkInScene(int x, int y) { return true; /* TODO */ }
bool CanWalkInScene(int x1, int y1, int x, int y) { return true; /* TODO */ }
void CheckEvent3() { /* TODO */ }
int StadySkillMenu(int x, int y, int w) { return -1; /* TODO */ }
int CommonMenu(int x, int y, int w, int max) { return CommonMenu(x, y, w, max, 0); }
int CommonMenu(int x, int y, int w, int max, int default_) { return -1; /* TODO */ }
void ShowCommonMenu(int x, int y, int w, int max, int menu) { /* TODO */ }
int CommonScrollMenu(int x, int y, int w, int max, int maxshow) { return -1; /* TODO */ }
void ShowCommonScrollMenu(int x, int y, int w, int max, int maxshow, int menu, int menutop) { /* TODO */ }
int CommonMenu2(int x, int y, int w) { return -1; /* TODO */ }
void ShowCommonMenu2(int x, int y, int w, int menu) { /* TODO */ }
int SelectOneTeamMember(int x, int y, const std::string& str, int list1, int list2) { return -1; /* TODO */ }
void MenuEsc() { /* TODO */ }
void ShowMenu(int menu) { /* TODO */ }
void MenuMedcine() { /* TODO */ }
void MenuMedPoision() { /* TODO */ }
bool MenuItem(int menu) { return false; /* TODO */ }
int ReadItemList(int ItemType) { return 0; /* TODO */ }
void ShowMenuItem(int row, int col, int x, int y, int atlu) { /* TODO */ }
void DrawItemFrame(int x, int y) { /* TODO */ }
void UseItem(int inum) { /* TODO */ }
bool CanEquip(int rnum, int inum) { return false; /* TODO */ }
void MenuStatus() { /* TODO */ }
void ShowStatus(int rnum) { /* TODO */ }
void MenuSystem() { /* TODO */ }
void ShowMenuSystem(int menu) { /* TODO */ }
void MenuLoad() { /* TODO */ }
bool MenuLoadAtBeginning() { return false; /* TODO */ }
void MenuSave() { /* TODO */ }
void MenuQuit() { /* TODO */ }
bool MenuDifficult() { return true; /* TODO */ }
int TitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow) { return -1; /* TODO */ }
void ShowTitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow, int menu, int menutop) { /* TODO */ }
void EffectMedcine(int role1, int role2) { /* TODO */ }
void EffectMedPoision(int role1, int role2) { /* TODO */ }
void EatOneItem(int rnum, int inum) { /* TODO */ }
void CallEvent(int num) { /* TODO */ }
void ShowSaveSuccess() { /* TODO */ }
void CheckHotkey(uint32 key) { /* TODO */ }
void FourPets() { /* TODO */ }
bool PetStatus(int r, int& menu) { return false; /* TODO */ }
void ShowPetStatus(int r, int p) { /* TODO */ }
void DrawFrame(int x, int y, int w, uint32 color) { /* TODO */ }
void PetLearnSkill(int r, int s) { /* TODO */ }
void ResistTheater() { /* TODO */ }
void ShowSkillMenu(int menu) { /* TODO */ }