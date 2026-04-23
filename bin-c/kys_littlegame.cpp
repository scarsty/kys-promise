// kys_littlegame.cpp - 小游戏实现
// 对应 kys_littlegame.pas implementation

#include "kys_littlegame.h"
#include "kys_engine.h"
#include "kys_main.h"
#include "kys_event.h"
#include "kys_gfx.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ==== iif ====
int iif(bool cond, int TrueReturn, int FalseReturn)
{
    return cond ? TrueReturn : FalseReturn;
}

// ==== randomsnake ====
void randomsnake()
{
    int j = 0;
    while (j <= 0)
    {
        j = 1;
        RANX = rand() % 16;
        RANY = rand() % 10;
        for (int i = 0; i < static_cast<int>(snake.size()); i++)
        {
            if (RANX == snake[i].x && RANY == snake[i].y)
                j--;
        }
    }
    if (EatFemale < 7)
        drawPngPic(background_, 40 * RANX, 40 * RANY, 0); // femalepic placeholder
    else
        drawPngPic(background_, 40 * RANX, 40 * RANY, 0);
    // 画女蛇食物 (用femalepic)
    // drawPngPic(femalepic_[std::min(EatFemale, 7)], 40 * RANX, 40 * RANY, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== movesnake ====
int movesnake(int Edest)
{
    for (int i = static_cast<int>(snake.size()) - 1; i >= 1; i--)
        snake[i] = snake[i - 1];

    int result = 0;
    int l = 5;

    // 吃到食物
    if (snake[0].x == RANX && snake[0].y == RANY)
    {
        if (EatFemale >= 7) l = 1;
        int oldLen = static_cast<int>(snake.size());
        snake.resize(oldLen + l);
        EatFemale++;
        for (int i = 1; i <= l; i++)
        {
            snake[snake.size() - i].x = snake[snake.size() - (l + 1)].x;
            snake[snake.size() - i].y = snake[snake.size() - (l + 1)].y;
        }
        drawPngPic(snakepic_, 40 * RANX, 40 * RANY, 0);
        randomsnake();
    }

    if (std::abs(dest - Edest) == 2)
        Edest = dest;

    switch (Edest)
    {
    case 0: snake[0].y = iif(snake[0].y >= 0, snake[0].y - 1, 9); break;
    case 1: snake[0].x = iif(snake[0].x < 15, snake[0].x + 1, 15); break;
    case 2: snake[0].y = iif(snake[0].y < 9, snake[0].y + 1, 9); break;
    case 3: snake[0].x = iif(snake[0].x >= 0, snake[0].x - 1, 15); break;
    }

    drawPngPic(background_, 40 * snake[snake.size() - 1].x, 40 * snake[snake.size() - 1].y, 0);
    drawPngPic(Weipic_, 40 * snake[0].x, 40 * snake[0].y, 0);
    drawPngPic(snakepic_, 40 * snake[1].x, 40 * snake[1].y, 0);

    for (int i = 1; i < static_cast<int>(snake.size()); i++)
    {
        if ((snake[0].x == snake[i].x && snake[0].y == snake[i].y) ||
            snake[0].x < 0 || snake[0].x >= 16 || snake[0].y < 0 || snake[0].y >= 16)
        {
            result = 1;
        }
    }
    dest = Edest;
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    return result;
}

// ==== FemaleSnake ====
int FemaleSnake()
{
    EatFemale = 0;
    if (fs::exists(AppPath + GAME_file))
    {
        FILE* grp = nullptr;
        fopen_s(&grp, (AppPath + GAME_file).c_str(), "rb");
        if (grp)
        {
            background_ = GetPngPic(grp, 4);
            snakepic_ = GetPngPic(grp, 5);
            Weipic_ = GetPngPic(grp, 6);
            // femalepic 也从同一文件读取
            fclose(grp);
        }
    }
    dest = 1;
    for (int i = 0; i < 16; i++)
        for (int j = 0; j < 10; j++)
            drawPngPic(background_, 40 * i, 40 * j, 0);

    snake.resize(8);
    snake[0].x = 1; snake[0].y = 0;
    drawPngPic(Weipic_, 40 * snake[0].x, 40 * snake[0].y, 0);
    for (int i = 1; i < static_cast<int>(snake.size()); i++)
    {
        snake[i].x = 0; snake[i].y = 0;
        drawPngPic(snakepic_, 40 * snake[i].x, 40 * snake[i].y, 0);
    }
    randomsnake();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    uint32_t ori_time = SDL_GetTicks();
    bool iskey = true;
    event.key.key = SDLK_DOWN;
    int delay = 120;
    if (GetPetSkill(4, 2)) delay = 150;

    while (SDL_PollEvent(&event) || true)
    {
        uint32_t now = SDL_GetTicks();
        if ((now - ori_time) >= static_cast<uint32_t>(delay))
        {
            ori_time = SDL_GetTicks();
            if (movesnake(dest) == 1) { WaitAnyKey(); break; }
        }
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (iskey)
            {
                iskey = false;
                int d = -1;
                if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) d = 2;
                else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) d = 0;
                else if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) d = 3;
                else if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) d = 1;
                if (d >= 0 && movesnake(d) == 1) { WaitAnyKey(); goto done; }
            }
            break;
        case SDL_EVENT_KEY_UP:
            iskey = true;
            break;
        }
    }
done:
    if (background_.pic) SDL_DestroySurface(background_.pic);
    if (snakepic_.pic) SDL_DestroySurface(snakepic_.pic);
    if (Weipic_.pic) SDL_DestroySurface(Weipic_.pic);
    for (int i = 0; i < 8; i++)
        if (femalepic_[i]) { SDL_DestroySurface(femalepic_[i]); femalepic_[i] = nullptr; }
    return EatFemale;
}

// ==== showarrow (内部) ====
static double showarrow(SDL_Surface* arrowpic, int x, int y, int step, double degree)
{
    if (!arrowpic) return degree;
    double r = std::pow(2.0 * std::pow(arrowpic->h / 2.0, 2.0), 0.5);
    if (degree > 90 || degree < -90)
    {
        double angle = std::atan2(static_cast<double>(screen->h - y), static_cast<double>(x - screen->w / 2));
        degree = -(90.0 - angle * 180.0 / M_PI);
    }
    double angle = (std::abs(degree) - 45.0) * M_PI / 180.0;
    double x1 = r * std::cos(std::abs(angle));

    SDL_Surface* newarrowpic = rotozoomSurfaceXY(arrowpic, degree, 1, 1, 1);
    if (!newarrowpic) return degree;
    SDL_Rect dstrect;
    dstrect.x = static_cast<int>(std::round((screen->w / 2.0) - std::abs(x1) - step * std::sin(degree * M_PI / 180.0)));
    dstrect.y = static_cast<int>(std::round(screen->h - std::abs(x1) - step * std::cos(degree * M_PI / 180.0)));
    dstrect.w = newarrowpic->w;
    dstrect.h = newarrowpic->h;
    SDL_SetSurfaceColorKey(newarrowpic, true, 0);
    SDL_BlitSurface(newarrowpic, nullptr, screen, &dstrect);
    SDL_DestroySurface(newarrowpic);
    return degree;
}

// ==== showbow (内部) ====
static double showbow(SDL_Surface* bowpic, int x, int y, double degree)
{
    if (!bowpic) return degree;
    double r = std::pow(2.0 * std::pow(bowpic->h / 2.0, 2.0), 0.5);
    if (degree > 90 || degree < -90)
    {
        double angle = std::atan2(static_cast<double>(screen->h - y), static_cast<double>(x - screen->w / 2));
        degree = -(90.0 - angle * 180.0 / M_PI);
    }
    double angle = (std::abs(degree) - 45.0) * M_PI / 180.0;
    double x1 = r * std::cos(std::abs(angle));

    SDL_Surface* newbowpic = rotozoomSurfaceXY(bowpic, degree, 1, 1, 1);
    if (!newbowpic) return degree;
    SDL_Rect dstrect;
    dstrect.x = static_cast<int>(std::round((screen->w / 2.0) - std::abs(x1)));
    dstrect.y = static_cast<int>(std::round(screen->h - std::abs(x1)));
    dstrect.w = newbowpic->w;
    dstrect.h = newbowpic->h;
    SDL_SetSurfaceColorKey(newbowpic, true, 0);
    SDL_BlitSurface(newbowpic, nullptr, screen, &dstrect);
    SDL_DestroySurface(newbowpic);
    return degree;
}

// ==== showeagle (内部) ====
static void showeagle(SDL_Surface* eaglepic, int birdx)
{
    if (!eaglepic) return;
    SDL_Rect dstrect;
    dstrect.x = birdx - eaglepic->w / 2;
    dstrect.y = 20;
    dstrect.w = eaglepic->w;
    dstrect.h = eaglepic->h;
    SDL_BlitSurface(eaglepic, nullptr, screen, &dstrect);
}

// ==== CheckGoal (内部) ====
static bool CheckGoal(int birdx, int arrowstep, double degree)
{
    int x = static_cast<int>(std::round((screen->w / 2.0) - arrowstep * std::sin(degree * M_PI / 180.0) - 100 * std::sin(degree * M_PI / 180.0)));
    int y = static_cast<int>(std::round(screen->h - arrowstep * std::cos(degree * M_PI / 180.0) - 100 * std::cos(degree * M_PI / 180.0)));
    return (x > birdx - 20 && x < birdx + 20 && y > 35 && y < 65);
}

// ==== ShotEagle ====
bool ShotEagle(int aim, int chance)
{
    if (GetPetSkill(4, 2)) chance *= 2;
    int arrowspeed = 6, goal = 0, arrowstep = 0;
    double degree = 0, arrowdegree = 0;
    uint32_t time_ = 0;
    int readystate = 1, bombnum = -1;
    TPic Gamepic{};
    SDL_Surface* arrowpic = nullptr;
    SDL_Surface* EaglePic[2][4]{};
    SDL_Surface* BowPic[2]{};
    SDL_Surface* Bombpic_[12]{};

    if (fs::exists(AppPath + GAME_file))
    {
        FILE* grp = nullptr;
        fopen_s(&grp, (AppPath + GAME_file).c_str(), "rb");
        if (grp)
        {
            Gamepic = GetPngPic(grp, 1);
            // 分割 Eagle/Bow/Bomb/Arrow 从 Gamepic
            for (int i = 0; i < 8; i++)
            {
                EaglePic[i / 4][i % 4] = SDL_CreateSurface(86, 65, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
                SDL_Rect src{ (i % 4) * 86, (i / 4) * 65, 86, 65 };
                SDL_BlitSurface(Gamepic.pic, &src, EaglePic[i / 4][i % 4], nullptr);
                SDL_SetSurfaceColorKey(EaglePic[i / 4][i % 4], true, getpixel(EaglePic[i / 4][i % 4], 0, 0));
            }
            for (int i = 0; i < 2; i++)
            {
                BowPic[i] = SDL_CreateSurface(220, 220, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
                SDL_Rect src{ i * 220, 130, 220, 110 };
                SDL_SetSurfaceColorKey(BowPic[i], true, 0);
                SDL_BlitSurface(Gamepic.pic, &src, BowPic[i], nullptr);
            }
            for (int i = 0; i < 12; i++)
            {
                Bombpic_[i] = SDL_CreateSurface(95, 88, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
                SDL_Rect src{ (i % 6) * 95, (i / 6) * 88 + 240, 95, 88 };
                SDL_SetSurfaceColorKey(Bombpic_[i], true, 0);
                SDL_BlitSurface(Gamepic.pic, &src, Bombpic_[i], nullptr);
            }
            arrowpic = SDL_CreateSurface(200, 200, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
            {
                SDL_Rect src{ 440, 130, 200, 100 };
                SDL_SetSurfaceColorKey(arrowpic, true, 0);
                SDL_BlitSurface(Gamepic.pic, &src, arrowpic, nullptr);
            }
            SDL_DestroySurface(Gamepic.pic);
            Gamepic = GetPngPic(grp, 2);
            fclose(grp);
        }
    }

    drawPngPic(Gamepic, 0, 0, 0);
    degree = showbow(BowPic[readystate], 320, 0, degree);
    int birdspeed = static_cast<int>(std::pow(-1.0, rand() % 2)) * (20 + rand() % 30);
    int birdx = (420 - (EaglePic[0][0] ? EaglePic[0][0]->w / 2 : 43)) + (birdspeed > 0 ? -320 : 320);
    int birdstep = 0;
    int xm = 0, ym = 0;
    bool result = false;

    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    while (SDL_PollEvent(&event) || true)
    {
        drawPngPic(Gamepic, 0, 0, 0);
        if (goal >= aim) { result = true; WaitAnyKey(); break; }
        if (chance < 0) { result = false; break; }

        DrawRectangle(4, 414, 77, 22, 0, 0xFFFFFF, 30);
        int eagleIdx = 1 - (1 + (birdspeed > 0 ? 1 : (birdspeed < 0 ? -1 : 0))) / 2;
        showeagle(EaglePic[eagleIdx][birdstep], birdx);

        if (bombnum == -1)
        {
            time_++;
            if (arrowstep > 0 && time_ % arrowspeed == 0)
                arrowstep += 15;
            if (readystate == 0 && time_ % 10 == 0)
            {
                arrowspeed--;
                if (arrowspeed <= 1) arrowspeed = 1;
            }
            if (readystate == 0 || arrowstep > 0)
            {
                SDL_GetMouseState2(xm, ym);
                arrowdegree = showarrow(arrowpic, xm, ym, arrowstep, arrowdegree);
                int bar = (6 - arrowspeed) * 15;
                uint32_t barCol = 0x330000 * (6 - arrowspeed);
                DrawRectangle(5, 415, bar, 20, barCol, barCol, 100);
                if (CheckGoal(birdx, arrowstep, arrowdegree))
                    bombnum = 0;
                if (static_cast<int>(arrowstep * std::sqrt(2.0) - 200) > screen->h)
                {
                    arrowstep = 0;
                    arrowspeed = 6;
                }
            }
            if (time_ % 3 == 0)
            {
                birdx += birdspeed;
                birdstep = (birdstep + 1) % 4;
                int ew = EaglePic[0][0] ? EaglePic[0][0]->w / 2 : 43;
                if ((birdspeed > 0 && birdx - ew > screen->w + 20) || (birdspeed < 0 && birdx + ew < 0))
                {
                    birdspeed = static_cast<int>(std::pow(-1.0, rand() % 2)) * (20 + rand() % 30);
                    birdx = (420 - ew) + (birdspeed > 0 ? -320 : 320);
                    birdstep = 0;
                }
            }
        }
        else
        {
            if (bombnum > 11)
            {
                arrowstep = 0; arrowspeed = 6;
                birdspeed = static_cast<int>(std::pow(-1.0, rand() % 2)) * (20 + rand() % 30);
                int ew = EaglePic[0][0] ? EaglePic[0][0]->w / 2 : 43;
                birdx = (420 - ew) + (birdspeed > 0 ? -320 : 320);
                birdstep = 0; bombnum = -1; goal++;
            }
            else
            {
                SDL_Rect dr{ birdx - Bombpic_[bombnum]->w / 2, 70 - Bombpic_[bombnum]->h / 2,
                    Bombpic_[bombnum]->w, Bombpic_[bombnum]->h };
                SDL_BlitSurface(Bombpic_[bombnum], nullptr, screen, &dr);
                bombnum++;
                SDL_Delay(20 * GameSpeed / 10);
            }
        }

        showbow(BowPic[readystate], xm, ym, degree);

        // 显示得分/机会/目标
        std::wstring wg = L"得分：", wc = L"機會：", wt = L"目標：";
        DrawShadowText(reinterpret_cast<uint16_t*>(wg.data()), 500, 415, ColColor(0xFF), ColColor(0x6F));
        DrawShadowText(reinterpret_cast<uint16_t*>(wc.data()), 500, 393, ColColor(0xFF), ColColor(0x6F));
        DrawShadowText(reinterpret_cast<uint16_t*>(wt.data()), 500, 371, ColColor(0xFF), ColColor(0x6F));
        std::wstring sg = std::to_wstring(goal), sc = std::to_wstring(chance), st = std::to_wstring(aim);
        DrawShadowText(reinterpret_cast<uint16_t*>(sg.data()), 570, 415, ColColor(0xFF), ColColor(0x6F));
        DrawShadowText(reinterpret_cast<uint16_t*>(sc.data()), 570, 393, ColColor(0xFF), ColColor(0x6F));
        DrawShadowText(reinterpret_cast<uint16_t*>(st.data()), 570, 371, ColColor(0xFF), ColColor(0x6F));

        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4)
            {
                degree += 1; showbow(BowPic[readystate], xm, ym, degree);
                if (arrowstep <= 0) arrowdegree = degree;
            }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6)
            {
                degree -= 1; showbow(BowPic[readystate], xm, ym, degree);
                if (arrowstep <= 0) arrowdegree = degree;
            }
            if (event.key.key == SDLK_SPACE && arrowstep <= 0 && readystate == 1)
            {
                readystate = 0;
                degree = showbow(BowPic[readystate], xm, ym, degree);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_SPACE && arrowstep <= 0 && readystate == 0)
            {
                readystate = 1; chance--;
                arrowstep += arrowspeed;
                degree = showbow(BowPic[readystate], xm, ym, degree);
            }
            if (event.key.key == SDLK_ESCAPE) { result = false; goto shotdone; }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT && arrowstep <= 0 && readystate == 0)
            {
                readystate = 1; chance--;
                arrowstep += arrowspeed;
                degree = showbow(BowPic[readystate], xm, ym, 180);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT && arrowstep <= 0 && readystate == 1)
            {
                readystate = 0;
                degree = showbow(BowPic[readystate], xm, ym, 180);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            degree = showbow(BowPic[readystate], xm, ym, 180);
            break;
        }
        if (arrowstep <= 0) arrowdegree = degree;
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(5 * GameSpeed / 10);
    }
shotdone:
    for (int i = 0; i < 8; i++) if (EaglePic[i / 4][i % 4]) SDL_DestroySurface(EaglePic[i / 4][i % 4]);
    for (int i = 0; i < 2; i++) if (BowPic[i]) SDL_DestroySurface(BowPic[i]);
    for (int i = 0; i < 12; i++) if (Bombpic_[i]) SDL_DestroySurface(Bombpic_[i]);
    if (arrowpic) SDL_DestroySurface(arrowpic);
    if (Gamepic.pic) SDL_DestroySurface(Gamepic.pic);
    return result;
}

// ==== Acupuncture ====
bool Acupuncture(int n)
{
    int16_t AcupunctureList[1001]{};
    int chance = 3;
    if (GetPetSkill(4, 2)) chance = 6;
    std::vector<int16_t> goal(n), select(n);
    TPic Gamepic{};

    if (fs::exists(AppPath + GAME_file))
    {
        FILE* grp = nullptr;
        fopen_s(&grp, (AppPath + GAME_file).c_str(), "rb");
        if (grp) { Gamepic = GetPngPic(grp, 0); fclose(grp); }
    }
    {
        FILE* col = nullptr;
        fopen_s(&col, (AppPath + "list/Acupuncture.bin").c_str(), "rb");
        if (col) { fread(AcupunctureList, 1, 1000, col); fclose(col); }
    }

    Redraw();
    DrawRectangle(AcupunctureList[2], AcupunctureList[3],
        std::abs(AcupunctureList[4] - AcupunctureList[2]),
        std::abs(AcupunctureList[5] - AcupunctureList[3]), 0, ColColor(0xFF), 25);

    std::wstring word = L" 遊戲規則：**將亮起的穴位按順序點亮**有三次機會";
    if (GetPetSkill(4, 2)) word = L" 遊戲規則：**將亮起的穴位按順序點亮**有五次機會";
    DrawRectangle((AcupunctureList[2] + AcupunctureList[4]) / 2 - 115, 160, 250, 115, 0, ColColor(0xFF), 25);
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()),
        (AcupunctureList[2] + AcupunctureList[4]) / 2 - 120, 170, ColColor(0x5), ColColor(0x7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();

    drawPngPic(Gamepic, 0, 0, 395, 400, AcupunctureList[0], AcupunctureList[1], 0);
    int acunum = 0;
    for (int i = 3; i < 500; i++)
    {
        if (AcupunctureList[2 * i] == -1 || AcupunctureList[2 * i + 1] == -1) { acunum = i - 3; break; }
        drawPngPic(Gamepic, 40, 400, 20, 20,
            AcupunctureList[2 * i] - 10, AcupunctureList[2 * i + 1] - 10, 0);
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    word = L" 準備開始";
    DrawRectangle((AcupunctureList[2] + AcupunctureList[4]) / 2 - 45, 205, 90, 30, 0, ColColor(0xFF), 25);
    DrawShadowText(reinterpret_cast<uint16_t*>(word.data()),
        (AcupunctureList[2] + AcupunctureList[4]) / 2 - 60, 210, ColColor(0x5), ColColor(0x7));
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    WaitAnyKey();

    // 显示随机目标穴位
    drawPngPic(Gamepic, 0, 0, 395, 400, AcupunctureList[0], AcupunctureList[1], 0);
    for (int i = 0; i < acunum; i++)
        drawPngPic(Gamepic, 40, 400, 20, 20,
            AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    SDL_Delay(500);

    for (int i = 0; i < n; i++)
    {
        int r = rand() % acunum;
        goal[i] = static_cast<int16_t>(r);
        // 亮起
        drawPngPic(Gamepic,
            AcupunctureList[2 * (r + 3)] - 10 - AcupunctureList[0],
            AcupunctureList[2 * (r + 3) + 1] - 10 - AcupunctureList[1],
            20, 20, AcupunctureList[2 * (r + 3)] - 10, AcupunctureList[2 * (r + 3) + 1] - 10, 0);
        drawPngPic(Gamepic, 0, 400, 20, 20,
            AcupunctureList[2 * (r + 3)] - 10, AcupunctureList[2 * (r + 3) + 1] - 10, 0);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(800);
        // 暗下
        drawPngPic(Gamepic,
            AcupunctureList[2 * (r + 3)] - 10 - AcupunctureList[0],
            AcupunctureList[2 * (r + 3) + 1] - 10 - AcupunctureList[1],
            20, 20, AcupunctureList[2 * (r + 3)] - 10, AcupunctureList[2 * (r + 3) + 1] - 10, 0);
        drawPngPic(Gamepic, 40, 400, 20, 20,
            AcupunctureList[2 * (r + 3)] - 10, AcupunctureList[2 * (r + 3) + 1] - 10, 0);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(300);
    }

    int accu = 10;
    bool result = false;
    for (int trytime = 0; trytime < chance; trytime++)
    {
        Redraw();
        DrawRectangle(AcupunctureList[2], AcupunctureList[3],
            std::abs(AcupunctureList[4] - AcupunctureList[2]),
            std::abs(AcupunctureList[5] - AcupunctureList[3]), 0, ColColor(0xFF), 25);
        drawPngPic(Gamepic, 0, 0, 395, 400, AcupunctureList[0], AcupunctureList[1], 0);
        for (int i = 0; i < acunum; i++)
            drawPngPic(Gamepic, 40, 400, 20, 20,
                AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);

        DrawRectangle(AcupunctureList[2], AcupunctureList[3], 17 + (chance + 1) * 20, 28, 0, ColColor(0xFF), 50);
        for (int i = 0; i < chance; i++)
            DrawRectangle(AcupunctureList[2] + 17 + i * 20, AcupunctureList[3] + 7, 15, 15,
                ColColor(0x14), ColColor(0xFF), 0);
        DrawRectangle(AcupunctureList[2] + trytime * 20 + 17, AcupunctureList[3] + 7, 15, 15,
            ColColor(0x14), ColColor(0x14), 100);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

        int s = 0;
        while (true)
        {
            while (SDL_WaitEvent(&event))
            {
                CheckBasicEvent();
                if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                {
                    int xm, ym;
                    SDL_GetMouseState2(xm, ym);
                    for (int i = 0; i < acunum; i++)
                    {
                        if (xm <= AcupunctureList[2 * (i + 3)] + accu &&
                            xm >= AcupunctureList[2 * (i + 3)] - accu &&
                            ym >= AcupunctureList[2 * (i + 3) + 1] - accu &&
                            ym <= AcupunctureList[2 * (i + 3) + 1] + accu)
                        {
                            select[s] = static_cast<int16_t>(i);
                            s++;
                            // highlight
                            drawPngPic(Gamepic,
                                AcupunctureList[2 * (i + 3)] - 10 - AcupunctureList[0],
                                AcupunctureList[2 * (i + 3) + 1] - 10 - AcupunctureList[1],
                                20, 20, AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);
                            drawPngPic(Gamepic, 20, 400, 20, 20,
                                AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);
                            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                            SDL_Delay(200);
                            drawPngPic(Gamepic,
                                AcupunctureList[2 * (i + 3)] - 10 - AcupunctureList[0],
                                AcupunctureList[2 * (i + 3) + 1] - 10 - AcupunctureList[1],
                                20, 20, AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);
                            drawPngPic(Gamepic, 40, 400, 20, 20,
                                AcupunctureList[2 * (i + 3)] - 10, AcupunctureList[2 * (i + 3) + 1] - 10, 0);
                            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                        }
                    }
                    if (s > 0 && select[s - 1] != goal[s - 1]) goto acu_check;
                    if (s > n - 1) goto acu_check;
                }
            }
        acu_check:
            bool allMatch = true;
            for (int i1 = 0; i1 < n; i1++)
            {
                if (goal[i1] != select[i1]) { allMatch = false; s = 0; break; }
            }
            if (allMatch && s >= n)
            {
                SDL_Delay(200);
                word = L" 挑戰成功";
                DrawRectangle((AcupunctureList[2] + AcupunctureList[4]) / 2 - 45, 205, 90, 30, 0, ColColor(0xFF), 25);
                DrawShadowText(reinterpret_cast<uint16_t*>(word.data()),
                    (AcupunctureList[2] + AcupunctureList[4]) / 2 - 60, 210, ColColor(0x5), ColColor(0x7));
                SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
                WaitAnyKey();
                result = true;
                SDL_DestroySurface(Gamepic.pic);
                return true;
            }
            SDL_Delay(200);
            word = L" 挑戰失敗";
            DrawRectangle((AcupunctureList[2] + AcupunctureList[4]) / 2 - 45, 205, 90, 30, 0, ColColor(0xFF), 25);
            DrawShadowText(reinterpret_cast<uint16_t*>(word.data()),
                (AcupunctureList[2] + AcupunctureList[4]) / 2 - 60, 210, ColColor(0x5), ColColor(0x7));
            SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
            WaitAnyKey();
            break;
        }
    }
    SDL_Delay(1000);
    SDL_DestroySurface(Gamepic.pic);
    return result;
}

// ==== Lamp ====
bool Lamp(int c, int beginpic, int whitecount, int chance)
{
    if (GetPetSkill(4, 2)) c--;
    int r = c;
    int x = (screen->w - c * 50) / 2;
    int y = (screen->h - r * 50) / 2;
    int pic2 = beginpic + 1;
    int pic3 = beginpic + 2;
    int menu = 0;

    gamearray.resize(1);
    gamearray[0].resize(c * r);
    for (int i = 0; i < c * r; i++)
        gamearray[0][i] = static_cast<int16_t>(beginpic);
    for (int i = 0; i < whitecount; i++)
    {
        int temp = rand() % (c * r);
        while (temp == beginpic) temp = rand() % (c * r);
        gamearray[0][temp] = static_cast<int16_t>(pic2);
    }

    DrawRectangleWithoutFrame(x - 10, y - 10, c * 50 + 20, r * 50 + 20, 0, 60);
    for (int i = 0; i < c * r; i++)
    {
        DrawSPic(gamearray[0][i], x + (i % c) * 50, y + (i / c) * 50,
            x + (i % c) * 50, y + (i / c) * 50, 51, 51);
        if (menu == i)
            DrawSPic(pic3, x + (menu % c) * 50, y + (menu / c) * 50,
                x + (i % c) * 50, y + (i / c) * 50, 51, 51);
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

    bool result = false;
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        int xm, ym;
        auto toggleAt = [&](int idx)
        {
            auto flip = [&](int j) {
                gamearray[0][j] = (gamearray[0][j] == beginpic) ? static_cast<int16_t>(pic2) : static_cast<int16_t>(beginpic);
            };
            flip(idx);
            if (idx % c > 0) flip(idx - 1);
            if (idx % c < c - 1) flip(idx + 1);
            if (idx / c > 0) flip(idx - c);
            if (idx / c < r - 1) flip(idx + c);
        };

        switch (event.type)
        {
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if (xm > x && xm < x + 50 * c && ym > y && ym < y + 50 * r)
                menu = (xm - x) / 50 + ((ym - y) / 50) * c;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (xm > x && xm < x + 50 * c && ym > y && ym < y + 50 * r)
                    menu = (xm - x) / 50 + ((ym - y) / 50) * c;
                toggleAt(menu);
            }
            break;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE) { result = false; goto lamp_done; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu -= c;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu += c;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu--;
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu++;
            if (menu < 0) menu += c * r;
            if (menu > c * r - 1) menu -= c * r;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                toggleAt(menu);
            break;
        }

        for (int i = 0; i < c * r; i++)
        {
            DrawSPic(gamearray[0][i], x + (i % c) * 50, y + (i / c) * 50,
                x + (i % c) * 50, y + (i / c) * 50, 51, 51);
            if (menu == i)
                DrawSPic(pic3, x + (menu % c) * 50, y + (menu / c) * 50,
                    x + (i % c) * 50, y + (i / c) * 50, 51, 51);
        }
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);

        result = true;
        for (int i = 0; i < c * r; i++)
        {
            if (gamearray[0][i] != gamearray[0][0]) { result = false; break; }
        }
        if (result) { SDL_Delay(1000); break; }
    }
lamp_done:
    return result;
}

// ==== ExchangePic (内部) ====
static void ExchangePic(int p1, int p2)
{
    std::swap(gamearray[0][p1], gamearray[0][p2]);
    std::swap(gamearray[1][p1], gamearray[1][p2]);
}

// ==== SelectPoetry (内部) ====
static int SelectPoetry(int wx1, int wy1, int c, int Count, int len, int menu, int chance)
{
    int r1 = Count / c;
    int menu1 = 0;
    int xm = 0, ym = 0;
    for (int i = 0; i < Count; i++)
    {
        if (i == menu1)
            DrawRectangle(wx1 + (i % c) * 40 + 11, wy1 + (i / c) * 40 - 9, 39, 39, 0, ColColor(0, 5), 0);
        else
            DrawRectangle(wx1 + (i % c) * 40 + 11, wy1 + (i / c) * 40 - 9, 39, 39, 0, ColColor(0, 255), 0);
    }
    SDL_UpdateRect2(screen, 0, 0, 640, 440);

    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if ((xm > wx1 - 11) && (xm < wx1 - 11 + 40 * c) && (ym > wy1 - 9) && (ym < wy1 - 9 + 40 * r1))
                menu1 = ((xm - wx1 - 11) / 40) + ((ym - wy1 + 9) / 40) * c;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if ((xm > wx1 - 11) && (xm < wx1 - 11 + 40 * c) && (ym > wy1 - 9) && (ym < wy1 - 9 + 40 * r1))
                    menu1 = ((xm - wx1 - 11) / 40) + ((ym - wy1 + 9) / 40) * c;
                if ((menu1 < Count) && (menu < (int)gamearray[1].size()))
                {
                    int16_t w = gamearray[1][menu];
                    gamearray[1][menu] = gamearray[2][menu1];
                    gamearray[2][menu1] = w;
                    chance--;
                }
            }
            goto sel_done;
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE)
                goto sel_done;
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu1 -= c;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu1 += c;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu1--;
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu1++;
            if (menu1 < 0) menu1 += Count;
            if (menu1 > Count - 1) menu1 -= Count;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                if ((menu1 < Count) && (menu < (int)gamearray[1].size()))
                {
                    int16_t w = gamearray[1][menu];
                    gamearray[1][menu] = gamearray[2][menu1];
                    gamearray[2][menu1] = w;
                    chance--;
                    goto sel_done;
                }
            }
            break;
        }
        for (int i = 0; i < Count; i++)
        {
            if (i == menu1)
                DrawRectangle(wx1 + (i % c) * 40 + 11, wy1 + (i / c) * 40 - 9, 39, 39, 0, ColColor(0, 5), 0);
            else
                DrawRectangle(wx1 + (i % c) * 40 + 11, wy1 + (i / c) * 40 - 9, 39, 39, 0, ColColor(0, 255), 0);
        }
        SDL_UpdateRect2(screen, 0, 0, 640, 440);
    }
sel_done:
    return chance;
}

// ==== Poetry ====
bool Poetry(int talknum, int chance, int c, int Count)
{
    if (GetPetSkill(4, 2)) chance *= 2;

    int x = 20, y = 20, wy = 160, wy1 = 300;
    int menu = 0;
    int16_t wd[2] = {};

    // Read talk file
    FILE* fidx = nullptr; fopen_s(&fidx, (AppPath + TALK_IDX).c_str(), "rb");
    FILE* fgrp = nullptr; fopen_s(&fgrp, (AppPath + TALK_GRP).c_str(), "rb");
    if (!fidx || !fgrp) { if (fidx) fclose(fidx); if (fgrp) fclose(fgrp); return false; }

    int offset = 0, len = 0;
    if (talknum == 0)
    {
        fread(&len, 4, 1, fidx);
        offset = 0;
    }
    else
    {
        fseek(fidx, (talknum - 1) * 4, SEEK_SET);
        fread(&offset, 4, 1, fidx);
        fread(&len, 4, 1, fidx);
    }
    len = len - offset;
    std::vector<uint8_t> talkarray(len + 1, 0);
    fseek(fgrp, offset, SEEK_SET);
    fread(talkarray.data(), 1, len, fgrp);
    fclose(fidx);
    fclose(fgrp);

    for (int i = 0; i < len; i++)
    {
        talkarray[i] ^= 0xFF;
        if (talkarray[i] == 0xFF) talkarray[i] = 0;
    }

    uint16_t* poet = reinterpret_cast<uint16_t*>(talkarray.data());
    int wlen = len / 2;

    gamearray.resize(3);
    gamearray[0].resize(wlen);
    gamearray[1].resize(wlen);
    gamearray[2].resize(Count);

    for (int i = 0; i < wlen; i++) { gamearray[0][i] = 0; gamearray[1][i] = 0; }
    for (int i = 0; i < Count; i++) { gamearray[2][i] = 0x2020; }

    for (int i = 0; i < wlen; i++)
    {
        gamearray[0][i] = poet[i];
        int n = rand() % wlen;
        while (gamearray[1][n] != 0) n = rand() % wlen;
        gamearray[1][n] = gamearray[0][i];
    }

    int row = wlen / c;
    int r1 = row;

    Redraw();
    DrawRectangleWithoutFrame(x, y, 600, 400, 0, 60);

    int wx = CENTER_X - c * 20 - 12;
    int wx1 = CENTER_X - (Count / r1) * 20 - 12;
    DrawRectangle(wx1 + 11, wy1 - 9, (Count / r1) * 40 - 1, r1 * 40 - 1, 0, ColColor(0, 255), 0);
    DrawRectangle(wx + 11, wy - 9, c * 40 - 1, row * 40 - 1, 0, ColColor(0, 255), 0);
    DrawRectangle(CENTER_X - c * 20 - 1, y + 15, c * 40 - 1, 39, 0, ColColor(0, 255), 0);

    for (int i = 0; i < wlen; i++)
    {
        wd[0] = gamearray[1][i];
        DrawGBKShadowText(reinterpret_cast<const char*>(wd), wx + (i % c) * 40, wy + (i / c) * 40, ColColor(0, 5), ColColor(0, 7));
    }
    for (int i = 0; i < Count; i++)
    {
        wd[0] = gamearray[2][i];
        DrawGBKShadowText(reinterpret_cast<const char*>(wd), wx1 + (i % (Count / r1)) * 40, wy1 + (i / (Count / r1)) * 40, ColColor(0, 5), ColColor(0, 7));
    }
    DrawRectangle(wx + (menu % c) * 40 + 11, wy + (menu / c) * 40 - 9, 39, 39, 0, ColColor(0, 255), 0);

    std::wstring str = L"\u6A5F\u6703\uFF1A";
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), wx + 10, y + 25, ColColor(0, 5), ColColor(0, 7));
    str = std::to_wstring(chance);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), wx + 80, y + 25, ColColor(0, 5), ColColor(0, 7));

    SDL_UpdateRect2(screen, 0, 0, 640, 440);

    bool result = false;
    int xm = 0, ym = 0;
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE) { gamearray.clear(); return false; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu -= c;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu += c;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu--;
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu++;
            if (menu < 0) menu += c * row;
            if (menu > c * row - 1) menu -= c * row;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                chance = SelectPoetry(wx1, wy1, Count / r1, Count, wlen, menu, chance);
                SDL_UpdateRect2(screen, 0, 0, 640, 440);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if ((xm > wx - 11) && (xm < wx - 11 + 40 * c) && (ym > wy - 9) && (ym < wy - 9 + 40 * row))
                menu = ((xm - wx - 11) / 40) + ((ym - wy + 9) / 40) * c;
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                SDL_GetMouseState2(xm, ym);
                if ((xm > wx - 11) && (xm < wx - 11 + 40 * c) && (ym > wy - 9) && (ym < wy - 9 + 40 * r1))
                    menu = ((xm - wx - 11) / 40) + ((ym - wy + 9) / 40) * c;
                chance = SelectPoetry(wx1, wy1, Count / r1, Count, wlen, menu, chance);
                SDL_UpdateRect2(screen, 0, 0, 640, 440);
            }
            break;
        }

        Redraw();
        DrawRectangleWithoutFrame(x, y, 600, 400, 0, 60);
        DrawRectangle(wx1 + 11, wy1 - 9, (Count / r1) * 40 - 1, r1 * 40 - 1, 0, ColColor(0, 255), 0);
        DrawRectangle(wx + 11, wy - 9, c * 40 - 1, row * 40 - 1, 0, ColColor(0, 255), 0);
        DrawRectangle(CENTER_X - c * 20 - 1, y + 15, c * 40 - 1, 39, 0, ColColor(0, 255), 0);

        for (int i = 0; i < wlen; i++)
        {
            wd[0] = gamearray[1][i];
            DrawGBKShadowText(reinterpret_cast<const char*>(wd), wx + (i % c) * 40, wy + (i / c) * 40, ColColor(0, 5), ColColor(0, 7));
        }
        for (int i = 0; i < Count; i++)
        {
            wd[0] = gamearray[2][i];
            DrawGBKShadowText(reinterpret_cast<const char*>(wd), wx1 + (i % (Count / r1)) * 40, wy1 + (i / (Count / r1)) * 40, ColColor(0, 5), ColColor(0, 7));
        }
        DrawRectangle(wx + (menu % c) * 40 + 11, wy + (menu / c) * 40 - 9, 39, 39, 0, ColColor(0, 255), 0);

        str = L"\u6A5F\u6703\uFF1A";
        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), wx + 10, y + 25, ColColor(0, 5), ColColor(0, 7));
        str = std::to_wstring(chance);
        DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), wx + 80, y + 25, ColColor(0, 5), ColColor(0, 7));

        SDL_UpdateRect2(screen, 0, 0, 640, 440);

        result = true;
        for (int i = 0; i < Count; i++)
        {
            if (gamearray[0][i] != gamearray[2][i])
            {
                result = false;
                break;
            }
        }
        if (result)
        {
            SDL_Delay(1000);
            gamearray.clear();
            break;
        }
        if (chance == 0)
        {
            SDL_Delay(1000);
            result = true;
            for (int i = 0; i < Count; i++)
            {
                if (gamearray[0][i] != gamearray[2][i])
                {
                    result = false;
                    gamearray.clear();
                    break;
                }
            }
            break;
        }
    }
    return result;
}

// ==== rotoSpellPicture ====
bool rotoSpellPicture(int num, int chance)
{
    if (GetPetSkill(4, 2)) chance *= 2;

    gamearray.resize(2);
    gamearray[0].resize(25);
    gamearray[1].resize(25);

    int menu = 0, menu2 = -1;
    int x = 150, y = 5, w = 410, h = 440;
    int right = 0;

    TPic gamepic{};
    if (std::filesystem::exists(AppPath + GAME_file))
    {
        FILE* grpf = nullptr; fopen_s(&grpf, (AppPath + GAME_file).c_str(), "rb");
        gamepic = GetPngPic(grpf, num + 3);
        fclose(grpf);
    }

    for (int i = 0; i < 25; i++)
    {
        gamearray[0][i] = -1;
        gamearray[1][i] = rand() % 4;
    }
    for (int i = 0; i < 25; i++)
    {
        while (true)
        {
            int r = rand() % 25;
            if (gamearray[0][r] == -1)
            {
                gamearray[0][r] = i;
                break;
            }
        }
    }

    SDL_Surface* pic[4][25] = {};
    for (int i = 0; i < 25; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            SDL_Surface* temp = SDL_CreateSurface(80, 80, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
            pic[j][i] = SDL_CreateSurface(80, 80, SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
            SDL_Rect srcrect;
            srcrect.x = (i % 5) * 80;
            srcrect.y = (i / 5) * 80;
            srcrect.w = 80;
            srcrect.h = 80;
            SDL_BlitSurface(gamepic.pic, &srcrect, temp, nullptr);

            for (int i1 = 0; i1 < 80; i1++)
            {
                for (int i2 = 0; i2 < 80; i2++)
                {
                    switch (j)
                    {
                    case 0: putpixel(pic[j][i], i1, i2, getpixel(temp, i1, i2)); break;
                    case 1: putpixel(pic[j][i], i1, i2, getpixel(temp, i2, 79 - i1)); break;
                    case 2: putpixel(pic[j][i], i1, i2, getpixel(temp, 79 - i1, 79 - i2)); break;
                    case 3: putpixel(pic[j][i], i1, i2, getpixel(temp, 79 - i2, i1)); break;
                    }
                }
            }
            SDL_DestroySurface(temp);
        }
    }
    SDL_Surface* littlegamepic = rotozoomSurfaceXY(gamepic.pic, 0, 0.3, 0.3, 0);
    SDL_DestroySurface(gamepic.pic);

    bool result = false;
    int xm = 0, ym = 0;

    auto drawBoard = [&]()
    {
        DrawRectangle(x - 5, y - 5, w, h, 0, ColColor(255), 100);
        DrawRectangle(x - 5 - 140, y - 5, 130, 130, 0, ColColor(255), 100);
        SDL_Rect sr;
        sr.x = x - 140; sr.y = y; sr.w = 120; sr.h = 120;
        SDL_BlitSurface(littlegamepic, nullptr, screen, &sr);

        for (int i = 0; i < 25; i++)
        {
            SDL_Rect r;
            r.x = (i % 5) * 80 + x;
            r.y = (i / 5) * 80 + y + 30;
            r.w = 80; r.h = 80;
            SDL_BlitSurface(pic[gamearray[1][i]][gamearray[0][i]], nullptr, screen, &r);
        }
        if (menu2 > -1)
            DrawRectangle((menu2 % 5) * 80 + x, (menu2 / 5) * 80 + y + 30, 80, 80, 0, ColColor(0x64), 0);
        if (menu > -1)
            DrawRectangle((menu % 5) * 80 + x, (menu / 5) * 80 + y + 30, 80, 80, 0, ColColor(0xFF), 0);

        std::wstring word = L"\u6A5F\u6703";
        std::wstring word1 = L"\u547D\u4E2D";
        DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), x + 5, y + 5, ColColor(5), ColColor(7));
        DrawShadowText(reinterpret_cast<uint16_t*>(word1.data()), x + 200, y + 5, ColColor(5), ColColor(7));
        word = std::to_wstring(chance);
        word1 = std::to_wstring(right);
        DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), x + 55, y + 5, ColColor(5), ColColor(7));
        DrawShadowText(reinterpret_cast<uint16_t*>(word1.data()), x + 250, y + 5, ColColor(5), ColColor(7));
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    };

    // Show original picture briefly
    Redraw();
    DrawRectangle(x - 5, y - 5, w, h, 0, ColColor(255), 100);
    for (int i = 0; i < 25; i++)
    {
        SDL_Rect sr;
        sr.x = (i % 5) * 80 + x;
        sr.y = (i / 5) * 80 + y + 30;
        sr.w = 80; sr.h = 80;
        SDL_BlitSurface(pic[0][i], nullptr, screen, &sr);
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    SDL_Delay(2000);

    drawBoard();

    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        int menu1 = menu;
        switch (event.type)
        {
        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_ESCAPE)
            {
                if (menu2 > -1)
                    menu2 = -1;
                else
                {
                    result = false;
                    goto roto_cleanup;
                }
            }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu -= 5;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu += 5;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu--;
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu++;
            if (menu > 24) menu -= 25;
            if (menu < 0) menu += 25;
            if (menu1 > -1)
            {
                SDL_Rect sr;
                sr.x = (menu1 % 5) * 80 + x;
                sr.y = (menu1 / 5) * 80 + y + 30;
                sr.w = 80; sr.h = 80;
                SDL_BlitSurface(pic[gamearray[1][menu1]][gamearray[0][menu1]], nullptr, screen, &sr);
            }
            if (event.key.key == SDLK_SPACE)
            {
                if (menu2 > -1)
                {
                    ExchangePic(menu, menu2);
                    menu2 = -1;
                    chance--;
                }
                else if (menu > -1)
                    menu2 = menu;
            }
            if (event.key.key == SDLK_RETURN)
            {
                gamearray[1][menu]++;
                if (gamearray[1][menu] > 3) gamearray[1][menu] = 0;
                SDL_Rect sr;
                sr.x = (menu % 5) * 80 + x;
                sr.y = (menu / 5) * 80 + y + 30;
                sr.w = 80; sr.h = 80;
                SDL_BlitSurface(pic[gamearray[1][menu]][gamearray[0][menu]], nullptr, screen, &sr);
            }
            break;
        case SDL_EVENT_MOUSE_MOTION:
            SDL_GetMouseState2(xm, ym);
            if (menu > -1) menu1 = menu;
            if ((xm > x) && (xm < x - 5 + w) && (ym > y) && (ym < y - 5 + h))
            {
                menu = ((xm - x) / 80) + ((ym - y - 30) / 80) * 5;
                if (menu > 24) menu = -1;
                if (menu != menu1 && menu1 > -1)
                {
                    SDL_Rect sr;
                    sr.x = (menu1 % 5) * 80 + x;
                    sr.y = (menu1 / 5) * 80 + y + 30;
                    sr.w = 80; sr.h = 80;
                    SDL_BlitSurface(pic[gamearray[1][menu1]][gamearray[0][menu1]], nullptr, screen, &sr);
                }
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_RIGHT)
            {
                if (menu2 > -1)
                    menu2 = -1;
                else
                {
                    gamearray[1][menu]++;
                    if (gamearray[1][menu] > 3) gamearray[1][menu] = 0;
                    SDL_Rect sr;
                    sr.x = (menu % 5) * 80 + x;
                    sr.y = (menu / 5) * 80 + y + 30;
                    sr.w = 80; sr.h = 80;
                    SDL_BlitSurface(pic[gamearray[1][menu]][gamearray[0][menu]], nullptr, screen, &sr);
                }
            }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                if (menu2 > -1)
                {
                    ExchangePic(menu, menu2);
                    menu2 = -1;
                    chance--;
                }
                else if (menu > -1)
                    menu2 = menu;
            }
            break;
        }

        right = 0;
        for (int i = 0; i < 25; i++)
        {
            if (gamearray[0][i] == i && gamearray[1][i] == 0)
                right++;
        }
        drawBoard();

        if (right == 25) result = true;
        else result = false;
        if (result)
        {
            SDL_Delay(700);
            WaitAnyKey();
            break;
        }
        else if (chance == 0)
        {
            SDL_Delay(700);
            WaitAnyKey();
            break;
        }
    }

roto_cleanup:
    gamearray.clear();
    for (int i = 0; i < 25; i++)
        for (int j = 0; j < 4; j++)
            SDL_DestroySurface(pic[j][i]);
    SDL_DestroySurface(littlegamepic);
    return result;
}