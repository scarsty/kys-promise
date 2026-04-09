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
#include <map>
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
    // Read existing INI content
    std::map<std::string, std::map<std::string, std::string>> ini;
    std::vector<std::string> sectionOrder;
    {
        std::ifstream ifs(file);
        std::string line, curSec;
        while (std::getline(ifs, line))
        {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();
            if (line.empty()) continue;
            if (line.front() == '[')
            {
                auto pos = line.find(']');
                if (pos != std::string::npos)
                {
                    curSec = line.substr(1, pos - 1);
                    if (ini.find(curSec) == ini.end())
                        sectionOrder.push_back(curSec);
                    ini[curSec];
                }
            }
            else
            {
                auto eq = line.find('=');
                if (eq != std::string::npos)
                    ini[curSec][line.substr(0, eq)] = line.substr(eq + 1);
            }
        }
    }
    // Update value
    if (ini.find(section) == ini.end())
        sectionOrder.push_back(section);
    ini[section][key] = std::to_string(value);
    // Write back
    std::ofstream ofs(file);
    for (auto& sec : sectionOrder)
    {
        ofs << "[" << sec << "]\n";
        for (auto& [k, v] : ini[sec])
            ofs << k << "=" << v << "\n";
    }
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
    // 注: C++ 版本不使用 TFileVersionInfo, 版本字符串直接硬编码

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
// Pascal 文件格式:
//   偏移 0: Count (int32, 条目总数)
//   偏移 4..(Count+1)*4-1: 索引表, 每项 4 字节为该条目的结束偏移
//   条目 num 的数据: 从 address 到 address+12+len
//     4字节 x, 4字节 y, 4字节 black, 然后 len 字节 PNG 数据

TPic GetPngPic(FILE* f, int num)
{
    TPic result{};
    if (!f) return result;

    // 读取总数
    int32_t Count = 0;
    fseek(f, 0, SEEK_SET);
    fread(&Count, 4, 1, f);

    // 读取结束偏移 (索引表中 num+1 位置)
    int32_t endOffset = 0;
    fseek(f, (num + 1) * 4, SEEK_SET);
    fread(&endOffset, 4, 1, f);

    // 读取起始偏移
    int32_t address = 0;
    if (num == 0)
        address = (Count + 1) * 4;
    else
    {
        fseek(f, num * 4, SEEK_SET);
        fread(&address, 4, 1, f);
    }

    // 数据长度 = 结束偏移 - 起始偏移 - 12(x,y,black各4字节)
    int32_t len = endOffset - address - 12;

    // 定位到数据区，读取 x, y, black
    fseek(f, address, SEEK_SET);
    fread(&result.x, 4, 1, f);
    fread(&result.y, 4, 1, f);
    fread(&result.black, 4, 1, f);

    // 读取 PNG 数据
    if (len > 0)
    {
        std::vector<uint8_t> buf(len);
        fread(buf.data(), 1, len, f);
        result.pic = ReadPicFromByte(buf.data(), len);
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

bool InitialRole()
{
    int battlemode2 = BattleMode;
    std::string lan = "TC";
    int t = 0;
    for (int i = 1; i <= 6; i++)
    {
        LoadR(i);
        t = std::max<int>(gametime, t);
    }
    LoadR(0);
    gametime = std::max<int16_t>(gametime, static_cast<int16_t>(t));
    BattleMode = battlemode2;
    if (BattleMode > gametime)
        BattleMode = std::min<int>(gametime, 2);
    Where = 3;
    int x = 275, y = 250;
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    std::string str1;
    if (gametime > 0)
        str1 = "\xe9\x87\x91\xe5\x85\x88\xe7\x94\x9f"; // 金先生
    else
        str1 = "\xe5\x85\x88\xe7\x94\x9f"; // 先生

    std::string input_name = str1;
    bool result = EnterString(input_name, CENTER_X - 113, CENTER_Y + 10, 86, 100);
    str1 = input_name;
    if (fullscreen == 1)
    {
        Redraw();
        DrawTitlePic(0, x, y);
        DrawTitlePic(1, x, y);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
    if (str1.empty()) result = false;
    if (result)
    {
        std::string Name = str1;
        if (gametime == 0)
            Name = std::string("\xe9\x87\x91") + Name; // 金
        // Copy name into RRole[0].Name (GBK encoded, max 10 bytes)
        char* p0 = RRole[0].Name;
        for (int i = 0; i < 10; i++) p0[i] = 0;
        for (int i = 0; i < std::min<int>(static_cast<int>(Name.size()), 8); i++)
            p0[i] = Name[i];

        Redraw();
        result = RandomAttribute();
        if (result)
        {
            if (gametime > 0)
                result = MenuDifficult();
            if (result)
            {
                PlayBeginningMovie(26, 0);
                StartAmi();
            }
        }
    }
    return result;
}
int WalkInScene(int Open)
{
    uint32 next_time = SDL_GetTicks();
    uint32 next_time2 = next_time + 800;
    nowstep = -1;
    Where = 1;
    now2 = 0;
    resetpallet();
    int walking = 0;
    CurEvent = -1;
    int speed = 0;
    InitialScene();

    if (Open == 1)
    {
        Sx = BEGIN_Sx;
        Sy = BEGIN_Sy;
        Cx = Sx;
        Cy = Sy;
        CurEvent = BEGIN_EVENT;
        DrawScene();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        CallEvent(BEGIN_EVENT);
        CurEvent = -1;
    }
    DrawScene();
    ShowSceneName(CurScene);
    CheckEvent3();
    rs = 0;
    while (SDL_PollEvent(&event) || true)
    {
        if (Where >= 3) break;
        if (Where == 0) return 0;
        if (Sx > 63) Sx = 63;
        if (Sy > 63) Sy = 63;
        if (Sx < 0) Sx = 0;
        if (Sy < 0) Sy = 0;
        uint32 now = SDL_GetTicks();

        if (((Sx == RScene[CurScene].ExitX[0]) && (Sy == RScene[CurScene].ExitY[0])) ||
            ((Sx == RScene[CurScene].ExitX[1]) && (Sy == RScene[CurScene].ExitY[1])) ||
            ((Sx == RScene[CurScene].ExitX[2]) && (Sy == RScene[CurScene].ExitY[2])))
        {
            nowstep = -1;
            ReSetEntrance();
            Where = 0;
            resetpallet();
            return -1;
        }
        else if (static_cast<int>(now - next_time) > 0)
        {
            if (Water >= 0)
            {
                Water += 6;
                if (Water > 180) Water = 0;
            }
            if (Showanimation == 0)
            {
                for (int i = 0; i < 200; i++)
                    if ((DData[CurScene][i][8] != 0) || (abs(DData[CurScene][i][7]) < abs(DData[CurScene][i][6])))
                    {
                        int old = DData[CurScene][i][5];
                        DData[CurScene][i][5] = DData[CurScene][i][5] + 2 * ((DData[CurScene][i][5] > 0) ? 1 : ((DData[CurScene][i][5] < 0) ? -1 : 0));
                        if (abs(DData[CurScene][i][5]) > abs(DData[CurScene][i][6]))
                            DData[CurScene][i][5] = DData[CurScene][i][7];
                        UpdateScene(DData[CurScene][i][10], DData[CurScene][i][9], old, DData[CurScene][i][5]);
                    }
            }
            if (time_ >= 0)
            {
                if (static_cast<int>(now - next_time2) > 0)
                {
                    if (timeevent > 0) time_ = time_ - 1;
                    if (time_ < 0) CallEvent(timeevent);
                    next_time2 = now + 1000;
                }
            }
            next_time = now + 200;
            rs = 0;
            DrawScene();
            rs = 1;
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        }

        if (walking == 1)
        {
            if (nowstep >= 0)
            {
                if (linex[nowstep] - Sy < 0) SFace = 2;
                else if (linex[nowstep] - Sy > 0) SFace = 1;
                else if (liney[nowstep] - Sx > 0) SFace = 3;
                else SFace = 0;
                SStep++;
                if (SStep >= 7) SStep = 1;
                Sy = linex[nowstep];
                Sx = liney[nowstep];
                Redraw();
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                rs = 1;
                SDL_Delay((5 * GameSpeed) / 10);
                if (Water < 0) SDL_Delay((10 * GameSpeed) / 10);
                DrawScene();
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                CheckEvent3();
                if (RandomEvent > 0)
                    if (rand() % 100 == 0)
                    {
                        CallEvent(RandomEvent);
                        nowstep = -1;
                    }
                nowstep--;
            }
            else
            {
                walking = 0;
                rs = 1;
            }
        }

        if (walking == 2)
        {
            speed++;
            int Sx1 = Sx, Sy1 = Sy;
            if (speed == 1 || speed >= 5)
            {
                switch (SFace)
                {
                case 0: Sx1--; break;
                case 1: Sy1++; break;
                case 2: Sy1--; break;
                case 3: Sx1++; break;
                }
                SStep++;
                if (SStep == 7) SStep = 1;
                if (CanWalkInScene(Sx1, Sy1))
                {
                    Sx = Sx1;
                    Sy = Sy1;
                }
            }
            DrawScene();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            rs = 1;
            SDL_Delay((5 * GameSpeed) / 10);
            if (Water < 0) SDL_Delay((10 * GameSpeed) / 10);
            DrawScene();
            nowstep = -1;
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            CheckEvent3();
            if (RandomEvent > 0 && rand() % 100 == 0)
            {
                CallEvent(RandomEvent);
                nowstep = -1;
            }
        }

        CheckBasicEvent();
        int xm, ym;
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            walking = 0;
            speed = 0;
            rs = 1;
            if (event.key.key == SDLK_ESCAPE)
            {
                NewMenuEsc();
                if (Where == 0)
                {
                    if (RScene[CurScene].ExitMusic >= 0)
                    {
                        StopMP3();
                        PlayMP3(RScene[CurScene].ExitMusic, -1);
                    }
                    Redraw();
                    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                    return 0;
                }
                walking = 0;
            }
            if (event.key.key == SDLK_F5)
            {
                SwitchFullscreen();
                IniWriteInt(iniFilePath, "set", "fullscreen", fullscreen);
            }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                int x = Sx, y = Sy;
                switch (SFace)
                {
                case 0: x--; break;
                case 1: y++; break;
                case 2: y--; break;
                case 3: x++; break;
                }
                if (x >= 0 && x < 64 && y >= 0 && y < 64 && SData[CurScene][3][x][y] >= 0)
                {
                    CurEvent = SData[CurScene][3][x][y];
                    walking = 0;
                    if (DData[CurScene][CurEvent][2] >= 0)
                        CallEvent(DData[CurScene][SData[CurScene][3][x][y]][2]);
                }
                CurEvent = -1;
            }
            if (event.key.key == SDLK_F4)
            {
                if (gametime > 0)
                {
                    int menu = 0;
                    MenuString.clear();
                    MenuString.resize(2);
                    MenuEngString.resize(2);
                    MenuString[0] = L" \u56DE\u5408\u5236";
                    MenuString[1] = L" \u534A\u5373\u6642";
                    menu = CommonMenu(27, 30, 90, 1, BattleMode / 2);
                    if (menu >= 0)
                    {
                        BattleMode = std::min(2, menu * 2);
                        IniWriteInt(iniFilePath, "set", "battlemode", BattleMode);
                    }
                    MenuString.clear();
                    MenuEngString.clear();
                }
            }
            if (event.key.key == SDLK_F3)
            {
                int menu = 0;
                MenuString.clear();
                MenuString.resize(2);
                MenuEngString.resize(2);
                MenuString[0] = L" \u5929\u6C23\u7279\u6548\uFF1A\u958B";
                MenuEngString[0] = L" ";
                MenuString[1] = L" \u5929\u6C23\u7279\u6548\uFF1A\u95DC";
                MenuEngString[1] = L" ";
                menu = CommonMenu(27, 30, 180, 1, Effect);
                if (menu >= 0)
                {
                    Effect = menu;
                    IniWriteInt(iniFilePath, "set", "effect", Effect);
                }
                MenuString.clear();
                MenuEngString.clear();
            }
            if (event.key.key == SDLK_F1)
            {
                int menu = 0;
                MenuString.clear();
                MenuString.resize(2);
                MenuEngString.resize(2);
                MenuString[0] = L" \u7E41\u9AD4\u5B57";
                MenuEngString[0] = L" ";
                MenuString[1] = L" \u7C21\u9AD4\u5B57";
                MenuEngString[1] = L" ";
                menu = CommonMenu(27, 30, 90, 1, Simple);
                if (menu >= 0)
                {
                    Simple = menu;
                    IniWriteInt(iniFilePath, "set", "simple", Simple);
                }
                MenuString.clear();
                MenuEngString.clear();
            }
            if (event.key.key == SDLK_F2)
            {
                int menu = 0;
                MenuString.clear();
                MenuString.resize(3);
                MenuEngString.resize(3);
                MenuString[0] = L" \u904A\u6232\u901F\u5EA6\uFF1A\u5FEB";
                MenuEngString[0] = L" ";
                MenuString[1] = L" \u904A\u6232\u901F\u5EA6\uFF1A\u4E2D";
                MenuEngString[1] = L" ";
                MenuString[2] = L" \u904A\u6232\u901F\u5EA6\uFF1A\u6162";
                MenuEngString[2] = L" ";
                menu = CommonMenu(27, 30, 180, 2, std::min(GameSpeed / 10, 2));
                if (menu >= 0)
                {
                    if (menu == 0) GameSpeed = 1;
                    if (menu == 1) GameSpeed = 10;
                    if (menu == 2) GameSpeed = 20;
                    IniWriteInt(iniFilePath, "constant", "game_speed", GameSpeed);
                }
                MenuString.clear();
                MenuEngString.clear();
            }
            if (event.key.key == SDLK_F6)
            {
                SaveR(6);
                ShowSaveSuccess();
            }
            CheckHotkey(event.key.key);
            break;
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { SFace = 2; walking = 2; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { SFace = 1; walking = 2; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { SFace = 0; walking = 2; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { SFace = 3; walking = 2; }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                NewMenuEsc();
                nowstep = 0;
                walking = 0;
            }
            if (Where == 0)
            {
                if (RScene[CurScene].ExitMusic >= 0)
                {
                    StopMP3();
                    PlayMP3(RScene[CurScene].ExitMusic, -1);
                }
                Redraw();
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                return 0;
            }
            else if (event.button.button == SDL_BUTTON_LEFT)
            {
                SDL_GetMouseState2(xm, ym);
                if (walking == 0)
                {
                    walking = 1;
                    int Ayp = (-xm + CENTER_X + 2 * (ym + SData[CurScene][4][Sx][Sy]) - 2 * CENTER_Y + 18) / 36 + Sx;
                    int Axp = (xm - CENTER_X + 2 * (ym + SData[CurScene][4][Sx][Sy]) - 2 * CENTER_Y + 18) / 36 + Sy;
                    if (Ayp >= 0 && Ayp <= 63 && Axp >= 0 && Axp <= 63)
                    {
                        for (int i = 0; i < 64; i++)
                            for (int i1 = 0; i1 < 64; i1++)
                                FWay[i][i1] = -1;
                        findway(Sy, Sx);
                        Moveman(Sy, Sx, Axp, Ayp);
                        nowstep = FWay[Axp][Ayp] - 1;
                        rs = 1;
                    }
                    else
                    {
                        walking = 0;
                        rs = 1;
                    }
                }
            }
            break;
        }

        if (Water >= 0) SDL_Delay((5 * GameSpeed) / 10);
        else SDL_Delay((10 * GameSpeed) / 10);
        event.key.key = 0;
        event.button.button = 0;
    }
    instruct_14();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    if (RScene[CurScene].ExitMusic >= 0)
    {
        StopMP3();
        PlayMP3(RScene[CurScene].ExitMusic, -1);
    }
    return 0;
}
void ShowSceneName(int snum)
{
    std::wstring Scenename = GBKToUnicode(RScene[snum].Name);
    int len = static_cast<int>(strlen(RScene[snum].Name));
    DrawTextWithRect(reinterpret_cast<uint16_t*>(Scenename.data()), 320 - len * 5 + 7, 100, len * 10 + 6, ColColor(0, 5), ColColor(0, 7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    if (RScene[snum].EntranceMusic >= 0)
    {
        StopMP3();
        PlayMP3(RScene[snum].EntranceMusic, -1);
    }
    SDL_Delay((500 * GameSpeed) / 10);
}
bool CanWalkInScene(int x, int y)
{
    if (x < 0 || x > 63 || y < 0 || y > 63) return false;
    bool result = true;
    if (SData[CurScene][1][x][y] <= 0 && SData[CurScene][1][x][y] >= -2)
        result = true;
    else
        result = false;
    if (abs(SData[CurScene][4][x][y] - SData[CurScene][4][Sx][Sy]) > 10)
        result = false;
    if (SData[CurScene][3][x][y] >= 0 && result && DData[CurScene][SData[CurScene][3][x][y]][0] == 1)
        result = false;
    if ((SData[CurScene][0][x][y] >= 358 && SData[CurScene][0][x][y] <= 362) ||
        SData[CurScene][0][x][y] == 522 || SData[CurScene][0][x][y] == 1022 ||
        (SData[CurScene][0][x][y] >= 1324 && SData[CurScene][0][x][y] <= 1330) ||
        SData[CurScene][0][x][y] == 1348)
        result = false;
    return result;
}
bool CanWalkInScene(int x1, int y1, int x, int y)
{
    if (x < 0 || x > 63 || y < 0 || y > 63) return false;
    bool result = true;
    if (SData[CurScene][1][x][y] <= 0 && SData[CurScene][1][x][y] >= -2)
        result = true;
    else
        result = false;
    if (abs(SData[CurScene][4][x][y] - SData[CurScene][4][x1][y1]) > 10)
        result = false;
    if (SData[CurScene][3][x][y] >= 0 && result && DData[CurScene][SData[CurScene][3][x][y]][0] == 1)
        result = false;
    if ((SData[CurScene][0][x][y] >= 358 && SData[CurScene][0][x][y] <= 362) ||
        SData[CurScene][0][x][y] == 522 || SData[CurScene][0][x][y] == 1022 ||
        (SData[CurScene][0][x][y] >= 1324 && SData[CurScene][0][x][y] <= 1330) ||
        SData[CurScene][0][x][y] == 1348)
        result = false;
    return result;
}
void CheckEvent3()
{
    int enum_ = SData[CurScene][3][Sx][Sy];
    if (enum_ >= 0 && DData[CurScene][enum_][4] > 0)
    {
        CurEvent = enum_;
        nowstep = -1;
        CallEvent(DData[CurScene][enum_][4]);
        CurEvent = -1;
    }
}
int StadySkillMenu(int x, int y, int w)
{
    int menu = 0;
    int result = -1;
    display_imgFromSurface(SKILL_PIC, x, y, x, y, w + 1, 29);
    ShowCommonMenu2(x, y, w, menu);
    SDL_UpdateRect2(screen, x, y, w + 1, 29);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4 || event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6)
            {
                menu = (menu == 1) ? 0 : 1;
                display_imgFromSurface(SKILL_PIC, x, y, x, y, w + 1, 29);
                ShowCommonMenu2(x, y, w, menu);
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE && Where <= 2)
            {
                result = -1;
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
                goto done_stady;
            }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                result = menu;
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
                goto done_stady;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT && Where <= 2)
            {
                result = -1;
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
                goto done_stady;
            }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                result = menu;
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
                goto done_stady;
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym;
            SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + 29)
            {
                int menup = menu;
                menu = (xm - x - 2) / 50;
                if (menu > 1) menu = 1;
                if (menu < 0) menu = 0;
                if (menup != menu)
                {
                    display_imgFromSurface(SKILL_PIC, x, y, x, y, w + 1, 29);
                    ShowCommonMenu2(x, y, w, menu);
                    SDL_UpdateRect2(screen, x, y, w + 1, 29);
                }
            }
            break;
        }
        }
    }
done_stady:
    event.key.key = 0;
    event.button.button = 0;
    return result;
}
int CommonMenu(int x, int y, int w, int max) { return CommonMenu(x, y, w, max, 0); }
int CommonMenu(int x, int y, int w, int max, int default_)
{
    int menu = default_;
    int result = -1;
    ShowCommonMenu(x, y, w, max, menu);
    SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2)
            {
                menu++;
                if (menu > max) menu = 0;
                ShowCommonMenu(x, y, w, max, menu);
                SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
            }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8)
            {
                menu--;
                if (menu < 0) menu = max;
                ShowCommonMenu(x, y, w, max, menu);
                SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE)
            {
                result = -1;
                Redraw();
                SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
                goto done_cm;
            }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                result = menu;
                SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
                goto done_cm;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                result = -1;
                Redraw();
                SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
                goto done_cm;
            }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                int xm, ym;
                SDL_GetMouseState2(xm, ym);
                if (xm >= x && xm < x + w && ym > y && ym < y + max * 22 + 29)
                {
                    result = menu;
                    if (result >= 0) goto done_cm;
                }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym;
            SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + max * 22 + 29)
            {
                int menup = menu;
                menu = (ym - y - 2) / 22;
                if (menu > max) menu = max;
                if (menu < 0) menu = 0;
                if (menup != menu)
                {
                    ShowCommonMenu(x, y, w, max, menu);
                    SDL_UpdateRect2(screen, x, y, w + 1, max * 22 + 29);
                }
            }
            break;
        }
        }
    }
done_cm:
    event.key.key = 0;
    event.button.button = 0;
    return result;
}
void ShowCommonMenu(int x, int y, int w, int max, int menu)
{
    Redraw();
    DrawRectangle(x, y, w, max * 22 + 28, 0, ColColor(255), 30);
    int p = MenuEngString.size() > 0 ? 1 : 0;
    for (int i = 0; i <= max; i++)
    {
        if (i == menu)
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 17, y + 2 + 22 * i, ColColor(0x64), ColColor(0x66));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * i, ColColor(0x64), ColColor(0x66));
        }
        else
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 17, y + 2 + 22 * i, ColColor(0x5), ColColor(0x7));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * i, ColColor(0x5), ColColor(0x7));
        }
    }
}
int CommonScrollMenu(int x, int y, int w, int max, int maxshow)
{
    int menu = 0, menutop = 0;
    int result = -1;
    maxshow = std::min(max + 1, maxshow);
    ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
    SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2)
            {
                menu++;
                if (menu - menutop >= maxshow) menutop++;
                if (menu > max) { menu = 0; menutop = 0; }
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8)
            {
                menu--;
                if (menu <= menutop) menutop = menu;
                if (menu < 0) { menu = max; menutop = menu - maxshow + 1; if (menutop < 0) menutop = 0; }
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_PAGEDOWN)
            {
                menu += maxshow; menutop += maxshow;
                if (menu > max) menu = max;
                if (menutop > max - maxshow + 1) menutop = max - maxshow + 1;
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_PAGEUP)
            {
                menu -= maxshow; menutop -= maxshow;
                if (menu < 0) menu = 0;
                if (menutop < 0) menutop = 0;
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE && Where <= 2)
            {
                result = -1; Redraw();
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
                goto done_csm;
            }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                result = menu; Redraw();
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
                goto done_csm;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT && Where <= 2)
            {
                result = -1; Redraw();
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
                goto done_csm;
            }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                int xm, ym; SDL_GetMouseState2(xm, ym);
                if (xm >= x && xm < x + w && ym > y && ym < y + max * 22 + 29)
                { result = menu; if (result >= 0) goto done_csm; }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + max * 22 + 29)
            {
                int menup = menu;
                menu = (ym - y - 2) / 22 + menutop;
                if (menu > max) menu = max;
                if (menu < 0) menu = 0;
                if (menup != menu)
                {
                    ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                    SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
                }
            }
            break;
        }
        }
    }
done_csm:
    event.key.key = 0;
    event.button.button = 0;
    return result;
}
void ShowCommonScrollMenu(int x, int y, int w, int max, int maxshow, int menu, int menutop)
{
    Redraw();
    int m = std::min(maxshow, max + 1);
    DrawRectangle(x, y, w, m * 22 + 6, 0, ColColor(255), 30);
    int p = MenuEngString.size() > 0 ? 1 : 0;
    for (int i = menutop; i < menutop + m; i++)
    {
        if (i == menu)
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 7, y + 2 + 22 * (i - menutop), ColColor(0x64), ColColor(0x66));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * (i - menutop), ColColor(0x64), ColColor(0x66));
        }
        else
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 7, y + 2 + 22 * (i - menutop), ColColor(0x5), ColColor(0x7));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * (i - menutop), ColColor(0x5), ColColor(0x7));
        }
    }
}
int CommonMenu2(int x, int y, int w)
{
    int menu = 0, result = -1;
    Redraw();
    ShowCommonMenu2(x, y, w, menu);
    SDL_UpdateRect2(screen, x, y, w + 1, 29);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6 || event.key.key == SDLK_KP_4)
            {
                menu = (menu == 1) ? 0 : 1;
                Redraw();
                ShowCommonMenu2(x, y, w, menu);
                SDL_UpdateRect2(screen, x, y, w + 1, 29);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE && Where <= 2) { result = -1; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, 29); goto done_cm2; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { result = menu; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, 29); goto done_cm2; }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT && Where <= 2) { result = -1; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, 29); goto done_cm2; }
            if (event.button.button == SDL_BUTTON_LEFT) {
                int xm, ym; SDL_GetMouseState2(xm, ym);
                if (xm >= x && xm < x + w && ym > y && ym < y + 29) { result = menu; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, 29); if (result >= 0) goto done_cm2; }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + 29) {
                int menup = menu;
                menu = (xm - x - 2) / 50;
                if (menu > 1) menu = 1; if (menu < 0) menu = 0;
                if (menup != menu) { Redraw(); ShowCommonMenu2(x, y, w, menu); SDL_UpdateRect2(screen, x, y, w + 1, 29); }
            }
            break;
        }
        }
    }
done_cm2:
    event.key.key = 0;
    event.button.button = 0;
    return result;
}
void ShowCommonMenu2(int x, int y, int w, int menu)
{
    DrawRectangle(x, y, w, 28, 0, ColColor(255), 30);
    for (int i = 0; i <= 1; i++)
    {
        if (i == menu)
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 17 + i * 50, y + 2, ColColor(0x64), ColColor(0x66));
        else
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 17 + i * 50, y + 2, ColColor(0x5), ColColor(0x7));
    }
}
int SelectOneTeamMember(int x, int y, const std::string& str, int list1, int list2)
{
    MenuString.clear();
    MenuString.resize(6);
    if (!str.empty())
        MenuEngString.resize(6);
    else
        MenuEngString.clear();
    int amount = 0;
    for (int i = 0; i < 6; i++)
    {
        if (TeamList[i] >= 0)
        {
            MenuString[i] = GBKToUnicode(RRole[TeamList[i]].Name);
            if (!str.empty())
            {
                wchar_t buf[64] = {};
                if (str.find("%4d/%4d") != std::string::npos)
                    swprintf(buf, 64, L"%4d/%4d", RRole[TeamList[i]].Data[list1], RRole[TeamList[i]].Data[list2]);
                else
                    swprintf(buf, 64, L"%3d", RRole[TeamList[i]].Data[list1]);
                MenuEngString[i] = buf;
            }
            amount++;
        }
    }
    int result;
    if (str.empty())
        result = CommonMenu(x, y, 85, amount - 1);
    else
        result = CommonMenu(x, y, 85 + static_cast<int>(MenuEngString[0].size()) * 10, amount - 1);
    return result;
}
void MenuEsc()
{
    int menu = 0;
    while (menu >= 0)
    {
        MenuString.clear();
        MenuString.resize(8);
        MenuEngString.resize(8);
        MenuString[0] = L" \u72C0\u614B"; // 狀態
        MenuString[1] = L" \u7269\u54C1"; // 物品
        MenuString[2] = L" \u6B66\u5B78"; // 武學
        MenuString[3] = L" \u6280\u80FD"; // 技能
        MenuString[4] = L" \u5167\u529F"; // 內功
        MenuString[5] = L" \u96E2\u968A"; // 離隊
        MenuString[6] = L" \u7CFB\u7D71"; // 系統
        MenuString[7] = L" \u8AAA\u660E"; // 說明
        menu = CommonMenu(27, 30, 46, 7, menu);
        switch (menu)
        {
        case 0:
            SelectShowStatus();
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case 1:
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case 2:
            SelectShowMagic();
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case 3:
            FourPets();
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case 4: break;
        case 5:
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        case 6:
            NewMenuSystem();
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            menu = -1; // break outer while
            break;
        case 7:
            ResistTheater();
            Redraw();
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            break;
        }
    }
}
void ShowMenu(int menu)
{
    std::wstring word[6] = {
        L" \u91AB\u7642", L" \u89E3\u6BD2", L" \u7269\u54C1",
        L" \u72C0\u614B", L" \u96E2\u968A", L" \u7CFB\u7D71"
    };
    int max = (Where == 0) ? 5 : 3;
    Redraw();
    DrawRectangle(27, 30, 46, max * 22 + 28, 0, ColColor(255), 30);
    for (int i = 0; i <= max; i++)
    {
        if (i == menu)
        {
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 11, 32 + 22 * i, ColColor(0x64));
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 10, 32 + 22 * i, ColColor(0x66));
        }
        else
        {
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 11, 32 + 22 * i, ColColor(0x5));
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 10, 32 + 22 * i, ColColor(0x7));
        }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}
void MenuMedcine()
{
    std::wstring str = L" \u968A\u54E1\u91AB\u7642\u80FD\u529B";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 80, 30, 132, ColColor(0x21), ColColor(0x23));
    int menu = SelectOneTeamMember(80, 65, "%3d", 46, 0);
    ShowMenu(0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    if (menu >= 0)
    {
        int role1 = TeamList[menu];
        str = L" \u968A\u54E1\u76EE\u524D\u751F\u547D";
        DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 80, 30, 132, ColColor(0x21), ColColor(0x23));
        menu = SelectOneTeamMember(80, 65, "%4d/%4d", 17, 18);
        int role2 = TeamList[menu];
        if (menu >= 0)
            EffectMedcine(role1, role2);
    }
    Redraw();
}
void MenuMedPoision()
{
    std::wstring str = L" \u968A\u54E1\u89E3\u6BD2\u80FD\u529B";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 80, 30, 132, ColColor(0x21), ColColor(0x23));
    int menu = SelectOneTeamMember(80, 65, "%3d", 48, 0);
    ShowMenu(1);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    if (menu >= 0)
    {
        int role1 = TeamList[menu];
        str = L" \u968A\u54E1\u4E2D\u6BD2\u7A0B\u5EA6";
        DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 80, 30, 132, ColColor(0x21), ColColor(0x23));
        menu = SelectOneTeamMember(80, 65, "%3d", 20, 0);
        int role2 = TeamList[menu];
        if (menu >= 0)
            EffectMedPoision(role1, role2);
    }
    Redraw();
}
bool MenuItem(int menu)
{
    int col = 6, row = 3;
    int x = 0, y = 0, atlu = 0;
    if (menu == 0) menu = 101;
    menu = menu - 1;
    int iamount = ReadItemList(menu);
    ShowMenuItem(row, col, x, y, atlu);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    bool result = true;
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                y++;
                if (y >= row) { if (ItemList[atlu + col * row] >= 0) atlu += col; y = row - 1; }
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                y--;
                if (y < 0) { y = 0; if (atlu > 0) atlu -= col; }
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (event.key.key == SDLK_PAGEDOWN) {
                atlu += col * row;
                if (ItemList[atlu + col * row] < 0 && iamount > col * row) {
                    y = y - (iamount - atlu) / col - 1 + row;
                    atlu = (iamount / col - row + 1) * col;
                    if (y >= row) y = row - 1;
                }
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (event.key.key == SDLK_PAGEUP) {
                atlu -= col * row;
                if (atlu < 0) { y += atlu / col; atlu = 0; if (y < 0) y = 0; }
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { x++; if (x >= col) x = 0; ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { x--; if (x < 0) x = col - 1; ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
            if (event.key.key == SDLK_ESCAPE) { result = true; SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); goto done_mi; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                int idx = y * col + x + atlu;
                CurItem = RItemList[ItemList[idx]].Number;
                if (Where != 2 && CurItem >= 0 && ItemList[idx] >= 0) UseItem(CurItem);
                iamount = ReadItemList(menu); ShowMenuItem(row, col, x, y, atlu);
                if (RItem[CurItem].ItemType != 0 && Where != 2) result = true;
                else { result = false; goto done_mi; }
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT) { result = false; SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); goto done_mi; }
            if (event.button.button == SDL_BUTTON_LEFT) {
                int idx = y * col + x + atlu;
                CurItem = RItemList[ItemList[idx]].Number;
                if (Where != 2 && CurItem >= 0 && ItemList[idx] >= 0) UseItem(CurItem);
                iamount = ReadItemList(menu); ShowMenuItem(row, col, x, y, atlu);
                if (RItem[CurItem].ItemType != 0 && Where != 2) result = true;
                else { result = false; goto done_mi; }
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm < 122 && Where != 2) goto done_mi;
            if (xm >= 110 && ym > 90 && ym < 316) {
                int xp = x, yp = y;
                x = (xm - 115) / 82; y = (ym - 95) / 82;
                if (x >= col) x = col - 1; if (y >= row) y = row - 1;
                if (x < 0) x = 0; if (y < 0) y = 0;
                if (x != xp || y != yp) { ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
            }
            if (xm >= 110 && xm < 612 && ym > 312) {
                y++; if (y >= row) { if (ItemList[atlu + col * row] >= 0) atlu += col; y = row - 1; }
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            if (xm >= 110 && xm < 612 && ym < 90) {
                if (atlu > 0) atlu -= col;
                ShowMenuItem(row, col, x, y, atlu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            }
            break;
        }
        }
    }
done_mi:
    return result;
}
int ReadItemList(int ItemType)
{
    int p = 0;
    for (int i = 0; i < 501; i++) ItemList[i] = -1;
    for (int i = 0; i < MAX_ITEM_AMOUNT; i++)
    {
        if (RItemList[i].Number >= 0)
        {
            if (Where == 2)
            {
                if (RItem[RItemList[i].Number].ItemType == 3 || RItem[RItemList[i].Number].ItemType == 4)
                { ItemList[p] = static_cast<int16_t>(i); p++; }
            }
            else if (RItem[RItemList[i].Number].ItemType == ItemType || ItemType == 100)
            { ItemList[p] = static_cast<int16_t>(i); p++; }
        }
    }
    return p;
}
void ShowMenuItem(int row, int col, int x, int y, int atlu)
{
    std::wstring words[5] = { L" \u5287\u60C5\u7269\u54C1", L" \u795E\u5175\u5BF6\u7532", L" \u6B66\u529F\u79D8\u7B08", L" \u9748\u4E39\u5999\u85E5", L" \u50B7\u4EBA\u6697\u5668" };
    std::wstring words2[23] = { L" \u751F\u547D", L" \u751F\u547D", L" \u4E2D\u6BD2", L" \u9AD4\u529B",
        L" \u5167\u529B", L" \u5167\u529B", L" \u5167\u529B", L" \u653B\u64CA", L" \u8F15\u529F",
        L" \u9632\u79A6", L" \u91AB\u7642", L" \u7528\u6BD2", L" \u89E3\u6BD2", L" \u6297\u6BD2",
        L" \u62F3\u638C", L" \u5FA1\u528D", L" \u800D\u5200", L" \u5947\u9580", L" \u6697\u5668",
        L" \u6B66\u5B78", L" \u54C1\u5FB7", L" \u5DE6\u53F3", L" \u5E36\u6BD2" };

    if (Where == 2)
        Redraw();
    else
        display_imgFromSurface(MENUITEM_PIC, 110, 0, 110, 0, 530, 440);
    DrawRectangle(110 + 12, 16, 499, 25, 0, ColColor(0, 255), 40);
    DrawRectangle(110 + 12, 46, 499, 25, 0, ColColor(0, 255), 40);
    DrawRectangle(110 + 12, 76, 499, 252, 0, ColColor(0, 255), 40);
    DrawRectangle(110 + 12, 335, 499, 86, 0, ColColor(0, 255), 40);
    for (int i1 = 0; i1 < row; i1++)
        for (int i2 = 0; i2 < col; i2++)
        {
            int listnum = ItemList[i1 * col + i2 + atlu];
            if (listnum >= 0 && listnum < MAX_ITEM_AMOUNT && RItemList[listnum].Number >= 0)
                DrawItemPic(RItemList[listnum].Number, i2 * 82 + 12 + 115, i1 * 82 + 95 - 14);
        }
    int listnum = ItemList[y * col + x + atlu];
    int item = (listnum >= 0 && listnum < MAX_ITEM_AMOUNT) ? RItemList[listnum].Number : -1;
    if (listnum >= 0 && listnum < MAX_ITEM_AMOUNT && RItemList[listnum].Amount > 0)
    {
        std::wstring str = std::format(L"{:5d}", RItemList[listnum].Amount);
        DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), 431 + 62 + 12, 32 - 14, ColColor(0, 0x64), ColColor(0, 0x66));
        int len = static_cast<int>(strlen(RItem[item].Name));
        DrawGBKShadowText(RItem[item].Name, 357 - len * 5 + 12, 32 - 14, ColColor(0, 0x21), ColColor(0, 0x23));
        len = static_cast<int>(strlen(RItem[item].Introduction));
        DrawGBKShadowText(RItem[item].Introduction, 357 - len * 5 + 12, 62 - 14, ColColor(0, 0x5), ColColor(0, 0x7));
        DrawShadowText(reinterpret_cast<uint16_t*>(words[RItem[item].ItemType].data()), 97 + 12, 315 + 36 - 14, ColColor(0, 0x21), ColColor(0, 0x23));
        if (RItem[item].User >= 0) {
            str = L" \u4F7F\u7528\u4EBA\uFF1A";
            DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 187 + 12, 315, ColColor(0, 0x21), ColColor(0, 0x23));
            DrawGBKShadowText(RRole[RItem[item].User].Name, 277 + 12, 315 + 36 - 14, ColColor(0, 0x64), ColColor(0, 0x66));
        }
        if (item == COMPASS_ID) {
            str = L" \u4F60\u7684\u4F4D\u7F6E\uFF1A";
            DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 187 + 12, 315 + 36 - 14, ColColor(0, 0x21), ColColor(0, 0x23));
            str = std::format(L"{:3d}, {:3d}", My, Mx);
            DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), 300 + 12, 315 + 36 - 14, ColColor(0, 0x64), ColColor(0, 0x66));
            str = L" \u8239\u7684\u4F4D\u7F6E\uFF1A";
            DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 387 + 12, 315 + 36 - 14, ColColor(0, 0x21), ColColor(0, 0x23));
            str = std::format(L"{:3d}, {:3d}", ShipX, ShipY);
            DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), 500 + 12, 315 + 36 - 14, ColColor(0, 0x64), ColColor(0, 0x66));
        }
    }
    // Show item stat bonuses
    if (listnum >= 0 && listnum < MAX_ITEM_AMOUNT && RItemList[listnum].Amount > 0 && item >= 0 && RItem[item].ItemType > 0)
    {
        int len2 = 0;
        int p2[23] = {};
        for (int i = 0; i < 23; i++) {
            if (RItem[item].Data[45 + i] != 0 && i != 4) { p2[i] = 1; len2++; }
        }
        if (RItem[item].ChangeMPType >= 0 && RItem[item].ChangeMPType <= 2) { p2[4] = 1; len2++; }
        int i1 = 0;
        for (int i = 0; i < 23; i++) {
            if (p2[i] == 1) {
                std::wstring str;
                if (i == 4) {
                    if (RItem[item].ChangeMPType == 0) str = L" \u967D";
                    else if (RItem[item].ChangeMPType == 1) str = L" \u9670";
                    else str = L" \u8ABF\u548C";
                } else if (RItem[item].Data[45 + i] > 0)
                    str = L"+" + std::format(L"{}", RItem[item].Data[45 + i]);
                else
                    str = std::format(L"{}", RItem[item].Data[45 + i]);
                DrawShadowText(reinterpret_cast<uint16_t*>(words2[i].data()), 97 + i1 % 5 * 98 + 12, i1 / 5 * 20 + 355, ColColor(0, 0x5), ColColor(0, 0x7));
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 147 + i1 % 5 * 98 + 12, i1 / 5 * 20 + 355, ColColor(0, 0x64), ColColor(0, 0x66));
                i1++;
            }
        }
    }
    DrawItemFrame(x, y);
}
void DrawItemFrame(int x, int y)
{
    for (int i = 0; i < 80; i++)
    {
        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 97 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 91 + 81 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 117 + 12, y * 82 + 95 + i - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 111 + 12 + 81, y * 82 + 95 + i - 14, ColColor(0, 255));

        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 96 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 92 + 81 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 116 + 12, y * 82 + 95 + i - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 112 + 12 + 81, y * 82 + 95 + i - 14, ColColor(0, 255));

        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 95 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 115 + 12 + i, y * 82 + 93 + 81 - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 115 + 12, y * 82 + 95 + i - 14, ColColor(0, 255));
        putpixel(screen, x * 82 + 113 + 12 + 81, y * 82 + 95 + i - 14, ColColor(0, 255));
    }
}
void UseItem(int inum)
{
    CurItem = inum;
    if (inum == MAP_ID) { ShowMap(); return; }
    switch (RItem[inum].ItemType)
    {
    case 0:
        if (RItem[inum].EventNum > 0)
            CallEvent(RItem[inum].EventNum);
        else if (Where == 1) {
            int x = Sx, y = Sy;
            switch (SFace) { case 0: x--; break; case 1: y++; break; case 2: y--; break; case 3: x++; break; }
            if (x >= 0 && x < 64 && y >= 0 && y < 64 && SData[CurScene][3][x][y] >= 0) {
                CurEvent = SData[CurScene][3][x][y];
                if (DData[CurScene][SData[CurScene][3][x][y]][3] >= 0)
                    CallEvent(DData[CurScene][SData[CurScene][3][x][y]][3]);
            }
            CurEvent = -1;
        }
        break;
    case 1: {
        int menu = SelectItemUser(inum);
        if (menu >= 0) {
            int rnum = menu;
            int p = RItem[inum].EquipType;
            if (CanEquip(rnum, inum)) {
                if (RRole[rnum].Equip[p] >= 0) {
                    if (RItem[RRole[rnum].Equip[p]].Magic > 0) {
                        RItem[RRole[rnum].Equip[p]].ExpOfMagic = static_cast<int16_t>(GetMagicLevel(rnum, RItem[RRole[rnum].Equip[p]].Magic));
                        StudyMagic(rnum, RItem[RRole[rnum].Equip[p]].Magic, 0, 0, 1);
                    }
                    RRole[rnum].MaxHP -= RItem[RRole[rnum].Equip[p]].AddMaxHP;
                    RRole[rnum].CurrentHP -= RItem[RRole[rnum].Equip[p]].AddMaxHP;
                    RRole[rnum].MaxMP -= RItem[RRole[rnum].Equip[p]].AddMaxMP;
                    RRole[rnum].CurrentMP -= RItem[RRole[rnum].Equip[p]].AddMaxMP;
                    instruct_32(RRole[rnum].Equip[p], 1);
                }
                instruct_32(inum, -1);
                RRole[rnum].Equip[p] = static_cast<int16_t>(inum);
                if (RItem[RRole[rnum].Equip[p]].Magic > 0)
                    StudyMagic(rnum, 0, RItem[RRole[rnum].Equip[p]].Magic, RItem[RRole[rnum].Equip[p]].ExpOfMagic, 1);
                RRole[rnum].MaxHP += RItem[RRole[rnum].Equip[p]].AddMaxHP;
                RRole[rnum].CurrentHP += RItem[RRole[rnum].Equip[p]].AddMaxHP;
                RRole[rnum].MaxMP += RItem[RRole[rnum].Equip[p]].AddMaxMP;
                RRole[rnum].CurrentMP += RItem[RRole[rnum].Equip[p]].AddMaxMP;
                RRole[rnum].CurrentMP = std::max<int16_t>(1, RRole[rnum].CurrentMP);
                RRole[rnum].CurrentHP = std::max<int16_t>(1, RRole[rnum].CurrentHP);
            } else {
                std::wstring str = L"\u3000\u3000\u3000\u3000\u3000\u6B64\u4EBA\u4E0D\u9069\u5408\u88DD\u5099\u6B64\u7269\u54C1";
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 162, 391, ColColor(0, 5), ColColor(0, 7));
                SDL_UpdateRect2(screen, 140, 391, 500, 25);
                WaitAnyKey();
            }
        }
        break;
    }
    case 2: {
        int menu = SelectItemUser(inum);
        if (menu >= 0) {
            int rnum = menu;
            if (CanEquip(rnum, inum)) {
                if (RRole[rnum].PracticeBook != inum) {
                    if (RRole[rnum].PracticeBook >= 0)
                        instruct_32(RRole[rnum].PracticeBook, 1);
                    instruct_32(inum, -1);
                    RRole[rnum].PracticeBook = static_cast<int16_t>(inum);
                    RRole[rnum].ExpForBook = 0;
                }
            } else {
                std::wstring str = L"\u3000\u3000\u3000\u3000\u3000\u6B64\u4EBA\u4E0D\u9069\u5408\u4FEE\u7149\u6B64\u79D8\u7B08";
                DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 162, 391, ColColor(0, 5), ColColor(0, 7));
                SDL_UpdateRect2(screen, 140, 391, 500, 25);
                WaitAnyKey();
            }
        }
        break;
    }
    case 3:
        if (RItem[inum].EventNum <= 0)
            SelectItemUser(inum);
        else
            CallEvent(RItem[inum].EventNum);
        break;
    case 4: break;
    }
}
bool CanEquip(int rnum, int inum)
{
    bool result = true;
    auto sign_ = [](int v) -> int { return (v > 0) ? 1 : ((v < 0) ? -1 : 0); };
    if (RItem[inum].needSex >= 0 && RItem[inum].needSex != RRole[rnum].Sexual) result = false;
    if (sign_(RItem[inum].NeedMP) * RRole[rnum].CurrentMP < RItem[inum].NeedMP) result = false;
    if (sign_(RItem[inum].NeedAttack) * GetRoleAttack(rnum, false) < RItem[inum].NeedAttack) result = false;
    if (sign_(RItem[inum].NeedSpeed) * GetRoleSpeed(rnum, false) < RItem[inum].NeedSpeed) result = false;
    if (sign_(RItem[inum].NeedUsePoi) * GetRoleUsePoi(rnum, false) < RItem[inum].NeedUsePoi) result = false;
    if (sign_(RItem[inum].NeedMedcine) * GetRoleMedcine(rnum, false) < RItem[inum].NeedMedcine) result = false;
    if (sign_(RItem[inum].NeedMedPoi) * GetRoleMedPoi(rnum, false) < RItem[inum].NeedMedPoi) result = false;
    if (sign_(RItem[inum].NeedFist) * GetRoleFist(rnum, false) < RItem[inum].NeedFist) result = false;
    if (sign_(RItem[inum].NeedSword) * GetRoleSword(rnum, false) < RItem[inum].NeedSword) result = false;
    if (sign_(RItem[inum].NeedKnife) * GetRoleKnife(rnum, false) < RItem[inum].NeedKnife) result = false;
    if (sign_(RItem[inum].NeedUnusual) * GetRoleUnusual(rnum, false) < RItem[inum].NeedUnusual) result = false;
    if (sign_(RItem[inum].NeedHidWeapon) * GetRoleHidWeapon(rnum, false) < RItem[inum].NeedHidWeapon) result = false;
    int Aptitude = (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1], RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 2) ? 100 : RRole[rnum].Aptitude;
    if (sign_(RItem[inum].NeedAptitude) * Aptitude < RItem[inum].NeedAptitude) result = false;
    if (RRole[rnum].MPType < 2 && RItem[inum].NeedMPType < 2)
        if (RRole[rnum].MPType != RItem[inum].NeedMPType) result = false;
    if (RItem[inum].OnlyPracRole >= 0 && result)
        result = (RItem[inum].OnlyPracRole == rnum);
    if (RItem[inum].Magic > 0) {
        int r = 0;
        for (int i = 0; i < 10; i++) if (RRole[rnum].Magic[i] > 0) r++;
        if (r >= 10 && RItem[inum].Magic > 0) result = false;
        for (int i = 0; i < 10; i++)
            if (RRole[rnum].Magic[i] == RItem[inum].Magic) { result = true; break; }
        if (GetMagicLevel(rnum, RItem[inum].Magic) >= 0 && RMagic[RItem[inum].Magic].MagicType == 5)
            result = false;
        else if (GetMagicLevel(rnum, RItem[inum].Magic) >= 900)
            result = false;
    }
    return result;
}
void MenuStatus()
{
    std::wstring str = L" \u67E5\u770B\u968A\u54E1\u72C0\u614B";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 80, 30, 132, ColColor(0x21), ColColor(0x23));
    int menu = SelectOneTeamMember(80, 65, "%3d", 15, 0);
    if (menu >= 0) {
        ShowStatus(TeamList[menu]);
        WaitAnyKey();
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    }
}
void ShowStatus(int rnum)
{
    std::wstring strs[22] = {
        L" \u7B49\u7D1A", L" \u751F\u547D", L" \u5167\u529B", L" \u9AD4\u529B", L" \u7D93\u9A57", L" \u5347\u7D1A",
        L" \u653B\u64CA", L" \u9632\u79A6", L" \u8F15\u529F", L" \u91AB\u7642\u80FD\u529B", L" \u7528\u6BD2\u80FD\u529B",
        L" \u89E3\u6BD2\u80FD\u529B", L" \u62F3\u638C\u529F\u592B", L" \u5FA1\u528D\u80FD\u529B", L" \u800D\u5200\u6280\u5DE7",
        L" \u5947\u9580\u5175\u5668", L" \u6697\u5668\u6280\u5DE7", L" \u88DD\u5099\u7269\u54C1", L" \u4FEE\u7149\u7269\u54C1",
        L" \u6240\u6703\u6B66\u529F", L" \u53D7\u50B7", L" \u4E2D\u6BD2" };
    Redraw();
    int x = 40, y = CENTER_Y - 160;
    DrawRectangle(x, y, 560, 315, 0, ColColor(255), 50);
    DrawHeadPic(RRole[rnum].HeadNum, x + 60, y + 80);
    std::wstring Name = GBKToUnicode(RRole[rnum].Name);
    int namelen = static_cast<int>(strlen(RRole[rnum].Name));
    DrawShadowText(reinterpret_cast<uint16_t*>(Name.data()), x + 68 - namelen * 5, y + 85, ColColor(0x64), ColColor(0x66));
    for (int i = 0; i < 6; i++) DrawShadowText(reinterpret_cast<uint16_t*>(strs[i].data()), x - 10, y + 110 + 21 * i, ColColor(0x21), ColColor(0x23));
    for (int i = 6; i <= 16; i++) DrawShadowText(reinterpret_cast<uint16_t*>(strs[i].data()), x + 160, y + 5 + 21 * (i - 6), ColColor(0x64), ColColor(0x66));
    DrawShadowText(reinterpret_cast<uint16_t*>(strs[19].data()), x + 360, y + 5, ColColor(0x21), ColColor(0x23));
    int addatk = 0, adddef = 0, addspeed = 0;
    for (int n = 0; n < 4; n++) {
        if (RRole[rnum].Equip[n] >= 0) {
            addatk += RItem[RRole[rnum].Equip[n]].AddAttack;
            adddef += RItem[RRole[rnum].Equip[n]].AddDefence;
            addspeed += RItem[RRole[rnum].Equip[n]].AddSpeed;
        }
    }
    auto fmtEng = [&](int val, int px, int py, uint32 c1, uint32 c2) {
        std::wstring s = std::format(L"{:4d}", val);
        DrawEngShadowText(reinterpret_cast<uint16_t*>(s.data()), px, py, c1, c2);
    };
    fmtEng(RRole[rnum].Attack + addatk, x + 300, y + 5 + 21 * 0, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Defence + adddef, x + 300, y + 5 + 21 * 1, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Speed + addspeed, x + 300, y + 5 + 21 * 2, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Medcine, x + 300, y + 5 + 21 * 3, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].UsePoi, x + 300, y + 5 + 21 * 4, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].MedPoi, x + 300, y + 5 + 21 * 5, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Fist, x + 300, y + 5 + 21 * 6, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Sword, x + 300, y + 5 + 21 * 7, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Knife, x + 300, y + 5 + 21 * 8, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].Unusual, x + 300, y + 5 + 21 * 9, ColColor(0x5), ColColor(0x7));
    fmtEng(RRole[rnum].HidWeapon, x + 300, y + 5 + 21 * 10, ColColor(0x5), ColColor(0x7));
    for (int i = 0; i < 10; i++) {
        int magicnum = RRole[rnum].Magic[i];
        if (magicnum > 0) {
            DrawGBKShadowText(RMagic[magicnum].Name, x + 360, y + 26 + 21 * i, ColColor(0x5), ColColor(0x7));
            std::wstring s = std::format(L"{:3d}", RRole[rnum].MagLevel[i] / 100 + 1);
            DrawEngShadowText(reinterpret_cast<uint16_t*>(s.data()), x + 520, y + 26 + 21 * i, ColColor(0x64), ColColor(0x66));
        }
    }
    fmtEng(RRole[rnum].Level, x + 110, y + 110, ColColor(0x5), ColColor(0x7));
    // HP with color based on hurt
    uint32 color1, color2;
    if (RRole[rnum].Hurt >= 67) { color1 = ColColor(0x14); color2 = ColColor(0x16); }
    else if (RRole[rnum].Hurt >= 34) { color1 = ColColor(0xE); color2 = ColColor(0x10); }
    else { color1 = ColColor(0x5); color2 = ColColor(0x7); }
    std::wstring str = std::format(L"{:4d}", RRole[rnum].CurrentHP);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 60, y + 131, color1, color2);
    str = L"/";
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 100, y + 131, ColColor(0x64), ColColor(0x66));
    // MaxHP color by poison
    if (RRole[rnum].Poision >= 67) { color1 = ColColor(0x35); color2 = ColColor(0x37); }
    else if (RRole[rnum].Poision >= 34) { color1 = ColColor(0x30); color2 = ColColor(0x32); }
    else { color1 = ColColor(0x21); color2 = ColColor(0x23); }
    str = std::format(L"{:4d}", RRole[rnum].MaxHP);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 110, y + 131, color1, color2);
    // MP color by type
    if (RRole[rnum].MPType == 1) { color1 = ColColor(0x4E); color2 = ColColor(0x50); }
    else if (RRole[rnum].MPType == 0) { color1 = ColColor(0x5); color2 = ColColor(0x7); }
    else { color1 = ColColor(0x63); color2 = ColColor(0x66); }
    str = std::format(L"{:4d}/{:4d}", RRole[rnum].CurrentMP, RRole[rnum].MaxMP);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 60, y + 152, color1, color2);
    str = std::format(L"{:4d}/{:4d}", RRole[rnum].PhyPower, MAX_PHYSICAL_POWER);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 60, y + 173, ColColor(0x5), ColColor(0x7));
    str = std::format(L"{:5d}", static_cast<uint16_t>(RRole[rnum].Exp));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 100, y + 194, ColColor(0x5), ColColor(0x7));
    if (RRole[rnum].Level == MAX_LEVEL)
        str = L"=";
    else
        str = std::format(L"{:5d}", static_cast<uint16_t>(LevelUpList[RRole[rnum].Level - 1]));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 100, y + 215, ColColor(0x5), ColColor(0x7));
    DrawShadowText(reinterpret_cast<uint16_t*>(strs[17].data()), x + 160, y + 240, ColColor(0x21), ColColor(0x23));
    DrawShadowText(reinterpret_cast<uint16_t*>(strs[18].data()), x + 360, y + 240, ColColor(0x21), ColColor(0x23));
    if (RRole[rnum].Equip[0] >= 0) DrawGBKShadowText(RItem[RRole[rnum].Equip[0]].Name, x + 170, y + 261, ColColor(0x5), ColColor(0x7));
    if (RRole[rnum].Equip[1] >= 0) DrawGBKShadowText(RItem[RRole[rnum].Equip[1]].Name, x + 170, y + 282, ColColor(0x5), ColColor(0x7));
    if (RRole[rnum].PracticeBook >= 0) {
        int mlevel = 1, magicnum = RItem[RRole[rnum].PracticeBook].Magic;
        if (magicnum > 0)
            for (int i = 0; i < 10; i++)
                if (RRole[rnum].Magic[i] == magicnum) { mlevel = RRole[rnum].MagLevel[i] / 100 + 1; break; }
        int Aptitude = (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1], RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 2) ? 100 : RRole[rnum].Aptitude;
        int needexp;
        if (RItem[RRole[rnum].PracticeBook].NeedExp > 0)
            needexp = mlevel * (RItem[RRole[rnum].PracticeBook].NeedExp * (8 - Aptitude / 15)) / 2;
        else
            needexp = mlevel * (RItem[RRole[rnum].PracticeBook].NeedExp * (1 + Aptitude / 15)) / 2;
        DrawGBKShadowText(RItem[RRole[rnum].PracticeBook].Name, x + 370, y + 261, ColColor(0x5), ColColor(0x7));
        if (mlevel == 10)
            str = std::format(L"{:5d}/=", static_cast<uint16_t>(RRole[rnum].ExpForBook));
        else
            str = std::format(L"{:5d}/{:5d}", static_cast<uint16_t>(RRole[rnum].ExpForBook), needexp);
        DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 400, y + 282, ColColor(0x64), ColColor(0x66));
    }
    SDL_UpdateRect2(screen, x, y, 561, 316);
}
void MenuSystem()
{
    int menu = 0;
    ShowMenuSystem(menu);
    while (SDL_WaitEvent(&event))
    {
        if (Where == 3) break;
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu++; if (menu > 3) menu = 0; ShowMenuSystem(menu); }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu--; if (menu < 0) menu = 3; ShowMenuSystem(menu); }
            if (event.key.key == SDLK_ESCAPE) { Redraw(); SDL_UpdateRect2(screen, 80, 30, 47, 95); goto done_ms; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                switch (menu) { case 3: MenuQuit(); break; case 1: MenuSave(); break; case 0: MenuLoad(); break;
                case 2: SwitchFullscreen(); IniWriteInt(iniFilePath, "set", "fullscreen", fullscreen); goto done_ms; }
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT) { Redraw(); SDL_UpdateRect2(screen, 80, 30, 47, 95); goto done_ms; }
            if (event.button.button == SDL_BUTTON_LEFT) {
                switch (menu) { case 3: MenuQuit(); break; case 1: MenuSave(); break; case 0: MenuLoad(); break;
                case 2: SwitchFullscreen(); IniWriteInt(iniFilePath, "set", "fullscreen", fullscreen); goto done_ms; }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION: {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 80 && xm < 127 && ym > 47 && ym < 120) {
                int menup = menu; menu = (ym - 32) / 22;
                if (menu > 3) menu = 3; if (menu < 0) menu = 0;
                if (menup != menu) ShowMenuSystem(menu);
            }
            break;
        }
        }
    }
done_ms:;
}
void ShowMenuSystem(int menu)
{
    std::wstring word[4] = { L" \u8B80\u53D6", L" \u5B58\u6A94", L" \u5168\u5C4F", L" \u96E2\u958B" };
    if (fullscreen == 1) word[2] = L" \u7A97\u53E3";
    Redraw();
    DrawRectangle(80, 30, 46, 92, 0, ColColor(255), 30);
    for (int i = 0; i <= 3; i++) {
        if (i == menu) {
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 64, 32 + 22 * i, ColColor(0x64));
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 63, 32 + 22 * i, ColColor(0x66));
        } else {
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 64, 32 + 22 * i, ColColor(0x5));
            DrawText_(screen, reinterpret_cast<uint16_t*>(word[i].data()), 63, 32 + 22 * i, ColColor(0x7));
        }
    }
    SDL_UpdateRect2(screen, 80, 30, 47, 93);
}
void MenuLoad()
{
    MenuString.clear(); MenuString.resize(5);
    MenuEngString.clear();
    MenuString[0] = L" \u9032\u5EA6\u4E00"; MenuString[1] = L" \u9032\u5EA6\u4E8C";
    MenuString[2] = L" \u9032\u5EA6\u4E09"; MenuString[3] = L" \u9032\u5EA6\u56DB"; MenuString[4] = L" \u9032\u5EA6\u4E94";
    int menu = CommonMenu(133, 30, 67, 4);
    if (menu >= 0) {
        LoadR(menu + 1);
        if (Where == 1) JmpScene(CurScene, Sy, Sx);
        Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        ShowMenu(5); ShowMenuSystem(0);
    }
}
bool MenuLoadAtBeginning()
{
    Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    MenuString.clear(); MenuString.resize(6);
    MenuEngString.clear();
    MenuString[0] = L" \u8F09\u5165\u9032\u5EA6\u4E00"; MenuString[1] = L" \u8F09\u5165\u9032\u5EA6\u4E8C";
    MenuString[2] = L" \u8F09\u5165\u9032\u5EA6\u4E09"; MenuString[3] = L" \u8F09\u5165\u9032\u5EA6\u56DB";
    MenuString[4] = L" \u8F09\u5165\u9032\u5EA6\u4E94"; MenuString[5] = L" \u8F09\u5165\u81EA\u52D5\u6A94";
    int menu = CommonMenu(265, 280, 107, 5);
    if (menu >= 0) { LoadR(menu + 1); return true; }
    return false;
}
void MenuSave()
{
    MenuString.clear(); MenuString.resize(5);
    MenuString[0] = L" \u9032\u5EA6\u4E00"; MenuString[1] = L" \u9032\u5EA6\u4E8C";
    MenuString[2] = L" \u9032\u5EA6\u4E09"; MenuString[3] = L" \u9032\u5EA6\u56DB"; MenuString[4] = L" \u9032\u5EA6\u4E94";
    int menu = CommonMenu(133, 30, 67, 4);
    if (menu >= 0) SaveR(menu + 1);
}
void MenuQuit()
{
    MenuString.clear(); MenuString.resize(2);
    MenuString[0] = L" \u53D6\u6D88"; MenuString[1] = L" \u78BA\u5B9A";
    int menu = CommonMenu(133, 30, 45, 1);
    if (menu == 1) Quit();
}
bool MenuDifficult()
{
    std::wstring str = L" \u9078\u64C7\u96E3\u5EA6";
    Redraw();
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 275, 270, 90, ColColor(0x21), ColColor(0x23));
    MenuString.clear(); MenuString.resize(6); MenuEngString.clear();
    MenuString[0] = L"   \u6975\u6613"; MenuString[1] = L"   \u5BB9\u6613";
    MenuString[2] = L"   \u4E2D\u6613"; MenuString[3] = L"   \u4E2D\u96E3";
    MenuString[4] = L"   \u56F0\u96E3"; MenuString[5] = L"   \u6975\u96E3";
    int menu = CommonMenu(275, 300, 90, std::min<int>(gametime, 5));
    if (menu >= 0) { RRole[0].difficulty = static_cast<int16_t>(menu * 20); return true; }
    return false;
}
int TitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow)
{
    int menu = 0, menutop = 0;
    int x = tx, y = ty + 30, w = tw;
    int result = -1;
    maxshow = std::min(max + 1, maxshow);
    ShowTitleCommonScrollMenu(word, color1, color2, tx, ty, tw, max, maxshow, menu, menutop);
    int h = std::min(maxshow * 22 + 29 + 8, screen->h - ty - 1);
    SDL_UpdateRect2(screen, tx, ty, tw + 1, h);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                menu++; if (menu - menutop >= maxshow) menutop++;
                if (menu > max) { menu = 0; menutop = 0; }
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                menu--; if (menu <= menutop) menutop = menu;
                if (menu < 0) { menu = max; menutop = menu - maxshow + 1; if (menutop < 0) menutop = 0; }
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_PAGEDOWN) {
                menu += maxshow; menutop += maxshow;
                if (menu > max) menu = max; if (menutop > max - maxshow + 1) menutop = max - maxshow + 1;
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            if (event.key.key == SDLK_PAGEUP) {
                menu -= maxshow; menutop -= maxshow;
                if (menu < 0) menu = 0; if (menutop < 0) menutop = 0;
                ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop);
                SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE && Where <= 2) { result = -1; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29); goto done_tcsm; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { result = menu; goto done_tcsm; }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT && Where <= 2) { result = -1; Redraw(); SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29); goto done_tcsm; }
            if (event.button.button == SDL_BUTTON_LEFT) { result = menu; goto done_tcsm; }
            break;
        case SDL_EVENT_MOUSE_MOTION: {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= x && xm < x + w && ym > y && ym < y + max * 22 + 29) {
                int menup = menu; menu = (ym - y - 2) / 22 + menutop;
                if (menu > max) menu = max; if (menu < 0) menu = 0;
                if (menup != menu) { ShowCommonScrollMenu(x, y, w, max, maxshow, menu, menutop); SDL_UpdateRect2(screen, x, y, w + 1, maxshow * 22 + 29); }
            }
            break;
        }
        }
    }
done_tcsm:
    event.key.key = 0; event.button.button = 0;
    return result;
}
void ShowTitleCommonScrollMenu(uint16_t* word, uint32 color1, uint32 color2, int tx, int ty, int tw, int max, int maxshow, int menu, int menutop)
{
    Redraw();
    int x = tx, y = ty + 30, w = tw;
    DrawRectangle(tx, ty, tw, 28, 0, ColColor(0, 255), 30);
    DrawShadowText(word, tx - 7, ty + 2, color1, color2);
    int m = std::min(maxshow, max + 1);
    DrawRectangle(x, y, w, m * 22 + 6, 0, ColColor(255), 30);
    int p = MenuEngString.size() > 0 ? 1 : 0;
    for (int i = menutop; i < menutop + m; i++) {
        if (i == menu) {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 7, y + 2 + 22 * (i - menutop), ColColor(0x64), ColColor(0x66));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * (i - menutop), ColColor(0x64), ColColor(0x66));
        } else {
            DrawShadowText(reinterpret_cast<uint16_t*>(MenuString[i].data()), x - 7, y + 2 + 22 * (i - menutop), ColColor(0x5), ColColor(0x7));
            if (p == 1 && i < static_cast<int>(MenuEngString.size()))
                DrawEngShadowText(reinterpret_cast<uint16_t*>(MenuEngString[i].data()), x + 73, y + 2 + 22 * (i - menutop), ColColor(0x5), ColColor(0x7));
        }
    }
}
void EffectMedcine(int role1, int role2)
{
    if (RRole[role1].PhyPower < 50) return;
    int addlife = GetRoleMedcine(role1, true) * (10 - RRole[role2].Hurt / 15) / 10;
    if (RRole[role2].Hurt - GetRoleMedcine(role1, true) > 20) addlife = 0;
    RRole[role2].Hurt -= static_cast<int16_t>(addlife / LIFE_HURT);
    if (RRole[role2].Hurt < 0) RRole[role2].Hurt = 0;
    if (addlife > RRole[role2].MaxHP - RRole[role2].CurrentHP)
        addlife = RRole[role2].MaxHP - RRole[role2].CurrentHP;
    RRole[role2].CurrentHP += static_cast<int16_t>(addlife);
    if (addlife > 0)
        if (!GetEquipState(role1, 1) && !GetGongtiState(role1, 1))
            RRole[role1].PhyPower -= 3;
}
void EffectMedPoision(int role1, int role2)
{
    if (RRole[role1].PhyPower < 50) return;
    int minuspoi = GetRoleMedPoi(role1, true);
    if (minuspoi < RRole[role2].Poision / 2) minuspoi = 0;
    else if (minuspoi > RRole[role2].Poision) minuspoi = RRole[role2].Poision;
    RRole[role2].Poision -= static_cast<int16_t>(minuspoi);
    if (minuspoi > 0)
        if (!GetEquipState(role1, 1) && !GetGongtiState(role1, 1))
            RRole[role1].PhyPower -= 3;
}
void EatOneItem(int rnum, int inum)
{
    std::wstring word[24] = {
        L" \u589E\u52A0\u751F\u547D", L" \u589E\u52A0\u751F\u547D\u6700\u5927\u503C", L" \u4E2D\u6BD2\u7A0B\u5EA6",
        L" \u589E\u52A0\u9AD4\u529B", L" \u5167\u529B\u9580\u8DEF\u6539\u53D8\u70BA", L" \u589E\u52A0\u5167\u529B",
        L" \u589E\u52A0\u5167\u529B\u6700\u5927\u503C", L" \u589E\u52A0\u653B\u64CA\u529B", L" \u589E\u52A0\u8F15\u529F",
        L" \u589E\u52A0\u9632\u79A6\u529B", L" \u589E\u52A0\u91AB\u7642\u80FD\u529B", L" \u589E\u52A0\u7528\u6BD2\u80FD\u529B",
        L" \u589E\u52A0\u89E3\u6BD2\u80FD\u529B", L" \u589E\u52A0\u6297\u6BD2\u80FD\u529B", L" \u589E\u52A0\u62F3\u638C\u80FD\u529B",
        L" \u589E\u52A0\u5FA1\u528D\u80FD\u529B", L" \u589E\u52A0\u800D\u5200\u80FD\u529B", L" \u589E\u52A0\u5947\u9580\u5175\u5668",
        L" \u589E\u52A0\u6697\u5668\u6280\u5DE7", L" \u589E\u52A0\u6B66\u5B78\u5E38\u8B58", L" \u589E\u52A0\u54C1\u5FB7\u6307\u6578",
        L" \u7FD2\u5F97\u5DE6\u53F3\u4E92\u640F", L" \u589E\u52A0\u653B\u64CA\u5E36\u6BD2", L" \u53D7\u50B7\u7A0B\u5EA6"
    };
    int rolelist[24] = { 17,18,20,21,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,58,57,19 };
    int addvalue[24] = {};
    for (int i = 0; i < 23; i++) addvalue[i] = RItem[inum].Data[45 + i];
    addvalue[23] = -(addvalue[0] / LIFE_HURT);
    if (-addvalue[23] > RRole[rnum].Data[19]) addvalue[23] = -RRole[rnum].Data[19];
    if (addvalue[1] + RRole[rnum].Data[18] > MAX_HP) addvalue[1] = MAX_HP - RRole[rnum].Data[18];
    if (addvalue[6] + RRole[rnum].Data[42] > MAX_MP) addvalue[6] = MAX_MP - RRole[rnum].Data[42];
    if (addvalue[1] + RRole[rnum].Data[18] < 0) addvalue[1] = -RRole[rnum].Data[18];
    if (addvalue[6] + RRole[rnum].Data[42] < 0) addvalue[6] = -RRole[rnum].Data[42];
    for (int i = 7; i <= 22; i++) {
        if (addvalue[i] + RRole[rnum].Data[rolelist[i]] > MaxProList[rolelist[i] - 43])
            addvalue[i] = MaxProList[rolelist[i] - 43] - RRole[rnum].Data[rolelist[i]];
        if (addvalue[i] + RRole[rnum].Data[rolelist[i]] < 0)
            addvalue[i] = -RRole[rnum].Data[rolelist[i]];
    }
    if (addvalue[0] + RRole[rnum].Data[17] > addvalue[1] + RRole[rnum].Data[18])
        addvalue[0] = addvalue[1] + RRole[rnum].Data[18] - RRole[rnum].Data[17];
    if (addvalue[2] + RRole[rnum].Data[20] < 0) addvalue[2] = -RRole[rnum].Data[20];
    if (addvalue[3] + RRole[rnum].Data[21] > MAX_PHYSICAL_POWER) addvalue[3] = MAX_PHYSICAL_POWER - RRole[rnum].Data[21];
    if (addvalue[5] + RRole[rnum].Data[41] > addvalue[6] + RRole[rnum].Data[42])
        addvalue[5] = addvalue[6] + RRole[rnum].Data[42] - RRole[rnum].Data[41];
    for (int i = 0; i <= 23; i++) {
        if (i != 4 && i != 21 && addvalue[i] != 0)
            RRole[rnum].Data[rolelist[i]] += static_cast<int16_t>(addvalue[i]);
        if (i == 4 && addvalue[i] >= 0 && RRole[rnum].Data[40] != 2)
            if (RRole[rnum].Data[rolelist[i]] != 2) RRole[rnum].Data[rolelist[i]] = static_cast<int16_t>(addvalue[i]);
        if (i == 21 && addvalue[i] == 1 && RRole[rnum].Data[rolelist[i]] != 1)
            RRole[rnum].Data[rolelist[i]] = 1;
    }
}
void CallEvent(int num)
{
    Cx = Sx; Cy = Sy; SStep = 0;
    FILE* idx = PasFileOpen(AppPath + "resource/kdef.idx", "rb");
    FILE* grp = PasFileOpen(AppPath + "resource/kdef.grp", "rb");
    if (!idx || !grp) { PasFileClose(idx); PasFileClose(grp); return; }
    int offset = 0, length = 0;
    if (num == 0) { offset = 0; PasFileRead(idx, &length, 4); }
    else { PasFileSeek(idx, (num - 1) * 4, SEEK_SET); PasFileRead(idx, &offset, 4); PasFileRead(idx, &length, 4); }
    length = (length - offset) / 2;
    std::vector<int16_t> e(length + 1, -1);
    PasFileSeek(grp, offset, SEEK_SET);
    PasFileRead(grp, e.data(), length * 2);
    PasFileClose(idx); PasFileClose(grp);
    int i = 0;
    while (i < static_cast<int>(e.size()) && e[i] >= 0)
    {
        switch (e[i])
        {
        case 0: i++; instruct_0(); continue;
        case 1: instruct_1(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 2: instruct_2(e[i+1], e[i+2]); i += 3; break;
        case 3: { int list[13]; for (int j = 0; j < 13; j++) list[j] = e[i+1+j]; instruct_3(list, 13); i += 14; break; }
        case 4: i += instruct_4(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 5: i += instruct_5(e[i+1], e[i+2]); i += 3; break;
        case 6: i += instruct_6(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 7: goto done_ce;
        case 8: instruct_8(e[i+1]); i += 2; break;
        case 9: i += instruct_9(e[i+1], e[i+2]); i += 3; break;
        case 10: instruct_10(e[i+1]); i += 2; break;
        case 11: i += instruct_11(e[i+1], e[i+2]); i += 3; break;
        case 12: instruct_12(); i++; break;
        case 13: instruct_13(); i++; break;
        case 14: instruct_14(); i++; break;
        case 15: instruct_15(); i++; goto done_ce;
        case 16: i += instruct_16(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 17: { int list[5]; for (int j = 0; j < 5; j++) list[j] = e[i+1+j]; instruct_17(list, 5); i += 6; break; }
        case 18: i += instruct_18(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 19: instruct_19(e[i+1], e[i+2]); i += 3; break;
        case 20: i += instruct_20(e[i+1], e[i+2]); i += 3; break;
        case 21: instruct_21(e[i+1]); i += 2; break;
        case 22: instruct_22(); i++; break;
        case 23: instruct_23(e[i+1], e[i+2]); i += 3; break;
        case 24: instruct_24(); i++; break;
        case 25: instruct_25(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 26: instruct_26(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5]); i += 6; break;
        case 27: instruct_27(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 28: i += instruct_28(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5]); i += 6; break;
        case 29: i += instruct_29(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5]); i += 6; break;
        case 30: instruct_30(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 31: i += instruct_31(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 32: instruct_32(e[i+1], e[i+2]); i += 3; break;
        case 33: instruct_33(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 34: instruct_34(e[i+1], e[i+2]); i += 3; break;
        case 35: instruct_35(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 36: i += instruct_36(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 37: instruct_37(e[i+1]); i += 2; break;
        case 38: instruct_38(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 39: instruct_39(e[i+1]); i += 2; break;
        case 40: instruct_40(e[i+1]); i += 2; break;
        case 41: instruct_41(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 42: i += instruct_42(e[i+1], e[i+2]); i += 3; break;
        case 43: i += instruct_43(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 44: instruct_44(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5], e[i+6]); i += 7; break;
        case 45: instruct_45(e[i+1], e[i+2]); i += 3; break;
        case 46: instruct_46(e[i+1], e[i+2]); i += 3; break;
        case 47: instruct_47(e[i+1], e[i+2]); i += 3; break;
        case 48: instruct_48(e[i+1], e[i+2]); i += 3; break;
        case 49: instruct_49(e[i+1], e[i+2]); i += 3; break;
        case 50: {
            int list[7]; for (int j = 0; j < 7; j++) list[j] = e[i+1+j];
            int p = instruct_50(list, 7); i += 8;
            if (p < 622592) i += p;
            else e[i + ((p + 32768) / 655360) - 1] = static_cast<int16_t>(p % 655360);
            break;
        }
        case 51: instruct_51(); i++; break;
        case 52: instruct_52(); i++; break;
        case 53: instruct_53(); i++; break;
        case 54: instruct_54(); i++; break;
        case 55: i += instruct_55(e[i+1], e[i+2], e[i+3], e[i+4]); i += 5; break;
        case 56: instruct_56(e[i+1]); i += 2; break;
        case 57: i++; break;
        case 58: instruct_58(); i++; break;
        case 59: instruct_59(); i++; break;
        case 60: i += instruct_60(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5]); i += 6; break;
        case 61: i += e[i+1]; i += 3; break;
        case 62: instruct_62(); i++; goto done_ce;
        case 63: instruct_63(e[i+1], e[i+2]); i += 3; break;
        case 64: instruct_64(); i++; break;
        case 65: i++; break;
        case 66: instruct_66(e[i+1]); i += 2; break;
        case 67: instruct_67(e[i+1]); i += 2; break;
        case 68: NewTalk0(e[i+1], e[i+2], e[i+3], e[i+4], e[i+5], e[i+6], e[i+7]); i += 8; break;
        case 69: ReSetName(e[i+1], e[i+2], e[i+3]); i += 4; break;
        case 70: ShowTitle(e[i+1], e[i+2]); i += 3; break;
        case 71: JmpScene(e[i+1], e[i+2], e[i+3]); i += 4; break;
        default: i++; break;
        }
    }
done_ce:
    event.key.key = 0; event.button.button = 0;
}
void ShowSaveSuccess()
{
    std::wstring str = L"  \u4FDD\u5B58\u6210\u529F";
    DrawTextWithRect(reinterpret_cast<uint16_t*>(str.data()), 320 - 50 + 7, 100, 100 + 6, ColColor(0, 5), ColColor(0, 7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();
}
void CheckHotkey(uint32 key)
{
    int k = static_cast<int>(key) - SDLK_1;
    if (k >= 0 && k < 6)
    {
        resetpallet(0);
        switch (k)
        {
        case 0: SelectShowStatus(); break;
        case 1: SelectShowMagic(); break;
        case 2: NewMenuItem(); break;
        case 3: NewMenuTeammate(); break;
        case 4: FourPets(); break;
        case 5: NewMenuSystem(); break;
        }
        resetpallet();
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        event.key.key = 0;
        event.button.button = 0;
    }
}
void FourPets()
{
    int r = 0, menu = 0;
    display_imgFromSurface(SKILL_PIC, 0, 0);
    ShowPetStatus(r + 1, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        int xm, ym;
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { r++; if (r >= RRole[0].PetAmount) r = 0; ShowPetStatus(r + 1, menu); }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { r--; if (r < 0) r = RRole[0].PetAmount - 1; ShowPetStatus(r + 1, menu); }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE) goto done_fp;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { if (!PetStatus(r + 1, menu)) goto done_fp; }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_RIGHT) goto done_fp;
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (xm >= 10 && xm < 90 && ym > 20 && ym < (RRole[0].PetAmount * 23) + 20) {
                    int r1 = r; r = (ym - 20) / 23;
                    if (r != r1) { if (!PetStatus(r + 1, menu)) goto done_fp; }
                }
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if (xm < 120) {
                if (xm >= 10 && xm < 90 && ym >= 20 && ym < (RRole[0].PetAmount * 23) + 20) {
                    int r1 = r; r = (ym - 20) / 23;
                    if (r != r1) ShowPetStatus(r + 1, menu);
                }
            } else { if (!PetStatus(r + 1, menu)) goto done_fp; }
            break;
        }
    }
done_fp:;
}
bool PetStatus(int r, int& menu)
{
    int x = 100 + 40, y = 180 - 60, w = 50;
    bool result = false;
    ShowPetStatus(r, menu);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        int xm, ym;
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { menu++; if (menu >= 5) menu = 0; result = true; ShowPetStatus(r, menu); }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { menu--; if (menu < 0) menu = 4; result = true; ShowPetStatus(r, menu); }
            if (event.key.key == SDLK_ESCAPE) goto done_ps;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) PetLearnSkill(r, menu);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_RIGHT) { result = false; goto done_ps; }
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (xm >= x && xm < x + w * 5 && ym > y && ym < y * 5) {
                    int menup = menu; menu = (xm - x) / w;
                    if (menu != menup) { result = false; ShowPetStatus(r, menu); }
                }
                PetLearnSkill(r, menu);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if (xm < 120) { result = true; goto done_ps; }
            else if (xm >= x && xm < x + w * 5 && ym > y && ym < y + w) {
                int menup = menu; menu = (xm - x) / w;
                result = false;
                if (menu != menup) ShowPetStatus(r, menu);
            }
            break;
        }
    }
done_ps:
    return result;
}
void ShowPetStatus(int r, int p)
{
    ShowSkillMenu(r - 1);
    int x = 100, y = 180, w = 50;
    display_imgFromSurface(SKILL_PIC, 120, 0, 120, 0, 520, 440);
    DrawHeadPic(r, x + 40, y - 60);
    std::wstring str;
    uint32 col1, col2;
    if (RRole[r].Magic[p] > 0) { RRole[r].Magic[p] = 1; str = L" \u5DF2\u7FD2\u5F97"; col1 = ColColor(255); col2 = ColColor(255); }
    else { str = L" \u672A\u7FD2\u5F97"; col1 = 0x808080; col2 = 0x808080; }
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 90 + 40, 320 - 60, col1, col2);
    str = L" \u5269\u9918\u6280\u80FD\u9EDE\u6578\uFF1A";
    RRole[0].AddSkillPoint = std::min<int16_t>(RRole[0].AddSkillPoint, 10);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 180 + 40, 130 - 60, ColColor(0, 5), ColColor(0, 7));
    int pts = RRole[0].AddSkillPoint + RRole[0].Level;
    for (int ri = 1; ri <= 5; ri++)
        for (int si = 0; si < 5; si++)
            pts -= RRole[ri].Magic[si] * (si + 1);
    str = std::format(L"{:3d}", pts);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 180 + 140 + 40, 130 - 60, ColColor(0, 5), ColColor(0, 7));
    for (int i = 0; i < 5; i++) {
        if (RRole[r].Magic[i] > 0)
            drawPngPic(SkillPIC[(r - 1) * 5 + i], i * w + x + 40, y - 60, 0);
        else
            DrawFrame(i * w + x + 1 + 40, y + 1 - 60, 39, ColColor(0, 0));
    }
    DrawFrame(p * w + x + 40, y - 60, 41, ColColor(255));
    str = L" \u6240\u9700\u6280\u80FD\u9EDE\u6578\uFF1A";
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 90 + 20, 290 - 60, ColColor(0, 5), ColColor(0, 7));
    str = std::format(L"{:3d}", p + 1);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 90 + 20 + 140, 290 - 60, ColColor(0, 5), ColColor(0, 7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}
void DrawFrame(int x, int y, int w, uint32 color)
{
    for (int i = 0; i <= w; i++) {
        putpixel(screen, x + i, y, color);
        putpixel(screen, x + i, y + w, color);
        putpixel(screen, x, y + i, color);
        putpixel(screen, x + w, y + i, color);
    }
}
void PetLearnSkill(int r, int s)
{
    int x = 100, y = 180, w = 50;
    if (RRole[r].Magic[s] == 0)
    {
        MenuString.clear();
        MenuString.resize(2);
        MenuString[0] = L" \u5B78\u7FD2";
        MenuString[1] = L" \u53D6\u6D88";
        int pts = RRole[0].AddSkillPoint + RRole[0].Level;
        for (int ri = 1; ri <= 5; ri++)
            for (int si = 0; si < 5; si++)
                pts -= RRole[ri].Magic[si] * (si + 1);
        if ((s == 0 || RRole[r].Magic[s - 1] > 0) && s < pts)
            if (StadySkillMenu(x + 30 + w * s, y + 18, 98) == 0)
                RRole[r].Magic[s] = 1;
    }
    MenuString.clear();
    ShowPetStatus(r, s);
}
void ResistTheater()
{
    // Empty in Pascal source
}
void ShowSkillMenu(int menu)
{
    display_imgFromSurface(SKILL_PIC, 10, 10, 10, 10, 110, 180);
    MenuString.clear();
    MenuString.resize(5);
    MenuString[0] = GBKToUnicode(RRole[1].Name);
    MenuString[1] = GBKToUnicode(RRole[2].Name);
    MenuString[2] = GBKToUnicode(RRole[3].Name);
    MenuString[3] = GBKToUnicode(RRole[4].Name);
    MenuString[4] = GBKToUnicode(RRole[5].Name);
    DrawRectangle(15, 16, 100, RRole[0].PetAmount * 23 + 10, 0, ColColor(0, 255), 40);
    for (int i = 0; i < RRole[0].PetAmount; i++) {
        if (i == menu) {
            DrawText_(screen, reinterpret_cast<uint16_t*>(MenuString[i].data()), 5, 20 + 23 * i, ColColor(0x64));
            DrawText_(screen, reinterpret_cast<uint16_t*>(MenuString[i].data()), 6, 20 + 23 * i, ColColor(0x66));
        } else {
            DrawText_(screen, reinterpret_cast<uint16_t*>(MenuString[i].data()), 5, 20 + 23 * i, ColColor(0x5));
            DrawText_(screen, reinterpret_cast<uint16_t*>(MenuString[i].data()), 6, 20 + 23 * i, ColColor(0x7));
        }
    }
}