// kys_engine.cpp - 引擎层实现
// 对应 kys_engine.pas 的 implementation 段

#include "kys_engine.h"
#include "kys_battle.h"
#include "kys_event.h"
#include "kys_main.h"
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
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---- 模块内部变量 ----
static MIX_Mixer* gMixer = nullptr;
static MIX_Track* MusicTrack = nullptr;
static MIX_Track* SfxTracks[10] = {};
static int SfxNextTrack = 0;

// ---- 辅助函数 ----

static bool EnsureMixerCreated()
{
    if (gMixer != nullptr) return true;
    if (!MIX_Init()) return false;
    SDL_AudioSpec spec{};
    spec.freq = 22050;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    gMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    return gMixer != nullptr;
}

static bool EnsureTrackForAudio(MIX_Track*& track, MIX_Audio* audio)
{
    return MIX_SetTrackAudio(track, audio);
}

static MIX_Track* AcquireSfxTrack(MIX_Audio* audio)
{
    if (audio == nullptr) return nullptr;
    int idx = SfxNextTrack;
    SfxNextTrack++;
    if (SfxNextTrack > 9) SfxNextTrack = 0;
    if (!EnsureTrackForAudio(SfxTracks[idx], audio))
        return nullptr;
    return SfxTracks[idx];
}

static void ApplyScenePalette(uint8_t& col1, uint8_t& col2, uint8_t& col3)
{
    if (Where != 1) return;
    if (CurScene < 0 || CurScene >= static_cast<int>(RScene.size())) return;
    switch (RScene[CurScene].Pallet)
    {
    case 1:
        col1 = static_cast<uint8_t>((69 * col1) / 100);
        col2 = static_cast<uint8_t>((73 * col2) / 100);
        col3 = static_cast<uint8_t>((75 * col3) / 100);
        break;
    case 2:
        col1 = static_cast<uint8_t>((85 * col1) / 100);
        col2 = static_cast<uint8_t>((75 * col2) / 100);
        col3 = static_cast<uint8_t>((30 * col3) / 100);
        break;
    case 3:
        col1 = static_cast<uint8_t>((25 * col1) / 100);
        col2 = static_cast<uint8_t>((68 * col2) / 100);
        col3 = static_cast<uint8_t>((45 * col3) / 100);
        break;
    }
}

// ==== 事件过滤 ====

bool EventFilter(void* p, SDL_Event* e)
{
    switch (e->type)
    {
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        return false;
    case SDL_EVENT_FINGER_MOTION:
        if (CellPhone == 0) return false;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        SDL_GetWindowSize(window, &RESOLUTIONX, &RESOLUTIONY);
        return false;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        PlayMP3(nowmusic, -1, 0);
        break;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        StopMP3();
        break;
    }
    return true;
}

// ==== 音频 ====

void InitialMusic()
{
    if (!EnsureMixerCreated()) return;

    MusicTrack = MIX_CreateTrack(gMixer);
    for (int i = 0; i < 10; i++)
        SfxTracks[i] = MIX_CreateTrack(gMixer);
    SfxNextTrack = 0;

    auto LoadMid = [](const std::string& filename) -> MIX_Audio*
    {
        auto id = SDL_CreateProperties();
        auto io = SDL_IOFromFile(filename.c_str(), "rb");
        SDL_SetPointerProperty(id, MIX_PROP_AUDIO_LOAD_IOSTREAM_POINTER, io);
        SDL_SetStringProperty(id, MIX_PROP_AUDIO_DECODER_STRING, "fluidsynth");
        std::string sf2 = AppPath + "music/mid.sf2";
        SDL_SetStringProperty(id, "SDL_mixer.decoder.fluidsynth.soundfont_path", sf2.c_str());
        auto result = MIX_LoadAudioWithProperties(id);
        SDL_CloseIO(io);
        SDL_DestroyProperties(id);
        return result;
    };

    for (int i = 0; i < 110; i++)
    {
        if (Music[i] != nullptr)
        {
            MIX_DestroyAudio(Music[i]);
            Music[i] = nullptr;
        }
        std::string str = AppPath + "music/" + std::to_string(i) + ".mp3";
        if (fs::exists(str))
            Music[i] = MIX_LoadAudio(gMixer, str.c_str(), false);
        else
        {
            str = AppPath + "music/" + std::to_string(i) + ".mid";
            if (fs::exists(str))
                Music[i] = LoadMid(str);
            else
                Music[i] = nullptr;
        }
    }

    for (int i = 0; i < 187; i++)
    {
        std::string str = AppPath + std::format("sound/e{:03d}.wav", i);
        if (ESound[i] != nullptr)
        {
            MIX_DestroyAudio(ESound[i]);
            ESound[i] = nullptr;
        }
        if (fs::exists(str))
            ESound[i] = MIX_LoadAudio(gMixer, str.c_str(), false);
        else
            ESound[i] = nullptr;
    }

    for (int i = 0; i < 100; i++)
    {
        std::string str = AppPath + std::format("sound/atk{:03d}.wav", i);
        if (ASound[i] != nullptr)
        {
            MIX_DestroyAudio(ASound[i]);
            ASound[i] = nullptr;
        }
        if (fs::exists(str))
            ASound[i] = MIX_LoadAudio(gMixer, str.c_str(), false);
        else
            ASound[i] = nullptr;
    }
}

void PlayMP3(int MusicNum, int times, int frombeginning)
{
    if (!EnsureMixerCreated()) return;
    int loops = (times == -1) ? -1 : 0;
    if (MusicNum >= 0 && MusicNum < 110 && MusicVolume > 0)
    {
        if (Music[MusicNum] != nullptr)
        {
            MIX_StopTrack(MusicTrack, 0);
            MIX_SetTrackAudio(MusicTrack, Music[MusicNum]);
            if (frombeginning == 1)
                MIX_SetTrackPlaybackPosition(MusicTrack, 0);
            MIX_SetTrackGain(MusicTrack, MusicVolume / 100.0f);
            MIX_SetTrackLoops(MusicTrack, loops);
            MIX_PlayTrack(MusicTrack, 0);
            nowmusic = MusicNum;
        }
    }
}

void StopMP3()
{
    if (MusicTrack != nullptr)
        MIX_StopTrack(MusicTrack, 0);
}

void PlaySoundE(int SoundNum, int times)
{
    int loops = (times == -1) ? -1 : 0;
    if (SoundNum >= 0 && SoundNum < 187 && SoundVolume > 0)
    {
        if (ESound[SoundNum] != nullptr)
        {
            auto track = AcquireSfxTrack(ESound[SoundNum]);
            if (track == nullptr) return;
            MIX_SetTrackGain(track, SoundVolume / 100.0f);
            MIX_SetTrackLoops(track, loops);
            MIX_PlayTrack(track, 0);
        }
    }
}

void PlaySoundE(int SoundNum)
{
    PlaySoundE(SoundNum, -1);
}

void PlaySound(int SoundNum, int times)
{
    PlaySoundE(SoundNum, times);
}

void PlaySoundEffect(int SoundNum, int times)
{
    int loops = (times == -1) ? -1 : 0;
    if (SoundNum >= 0 && SoundNum < 100 && SoundVolume > 0)
    {
        if (ASound[SoundNum] != nullptr)
        {
            auto track = AcquireSfxTrack(ASound[SoundNum]);
            if (track == nullptr) return;
            MIX_SetTrackGain(track, SoundVolume / 100.0f);
            MIX_SetTrackLoops(track, loops);
            MIX_PlayTrack(track, 0);
        }
    }
}

// ==== 基本绘图 ====

uint32 getpixel(SDL_Surface* surface, int x, int y)
{
    if (x >= 0 && x < screen->w && y >= 0 && y < screen->h)
    {
        auto* p = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(surface->pixels) + y * surface->pitch + x * 4);
        return *p;
    }
    return 0;
}

void putpixel(SDL_Surface* surface_, int x, int y, uint32 pixel)
{
    int regionx1 = 0, regionx2 = screen->w;
    int regiony1 = 0, regiony2 = screen->h;
    if (x >= regionx1 && x < regionx2 && y >= regiony1 && y < regiony2)
    {
        auto* p = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(surface_->pixels) + y * surface_->pitch + x * 4);
        *p = pixel;
    }
}

void drawscreenpixel(int x, int y, uint32 color)
{
    putpixel(screen, x, y, color);
    SDL_UpdateRect2(screen, x, y, 1, 1);
}

void display_bmp(const char* file_name, int x, int y)
{
    std::string path = AppPath + file_name;
    if (!fs::exists(path)) return;
    auto* image = SDL_LoadBMP(file_name);
    if (image == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_BlitSurface(image, nullptr, screen, &dest);
    SDL_DestroySurface(image);
}

void display_img(const char* file_name, int x, int y, int x1, int y1, int w, int h)
{
    std::string path = AppPath + file_name;
    if (!fs::exists(path)) return;
    auto* image = IMG_Load(file_name);
    if (image == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_Rect src{ x1, y1, w, h };
    SDL_BlitSurface(image, &src, screen, &dest);
    SDL_DestroySurface(image);
}

void display_img(const char* file_name, int x, int y)
{
    std::string path = AppPath + file_name;
    if (!fs::exists(path)) return;
    auto* image = IMG_Load(file_name);
    if (image == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_BlitSurface(image, nullptr, screen, &dest);
    SDL_DestroySurface(image);
}

void display_imgFromSurface(SDL_Surface* image, int x, int y, int x1, int y1, int w, int h)
{
    if (image == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_Rect src{ x1, y1, w, h };
    SDL_BlitSurface(image, &src, screen, &dest);
}

void display_imgFromSurface(TPic image, int x, int y, int x1, int y1, int w, int h)
{
    if (image.pic == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_Rect src{ x1, y1, w, h };
    SDL_BlitSurface(image.pic, &src, screen, &dest);
}

void display_imgFromSurface(SDL_Surface* image, int x, int y)
{
    if (image == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_BlitSurface(image, nullptr, screen, &dest);
}

void display_imgFromSurface(TPic image, int x, int y)
{
    if (image.pic == nullptr) return;
    SDL_Rect dest{ x, y, 0, 0 };
    SDL_BlitSurface(image.pic, nullptr, screen, &dest);
}

uint32 ColColor(int num)
{
    return SDL_MapSurfaceRGB(screen, ACol[num * 3] * 4, ACol[num * 3 + 1] * 4, ACol[num * 3 + 2] * 4);
}

uint32 ColColor(int colnum, int num)
{
    return SDL_MapSurfaceRGB(screen, Col[colnum][num * 3] * 4, Col[colnum][num * 3 + 1] * 4, Col[colnum][num * 3 + 2] * 4);
}

// ==== 判断是否在屏幕内 ====

bool JudgeInScreen(int px, int py, int w, int h, int xs, int ys)
{
    return (px - xs + w >= 0) && (px - xs < screen->w) && (py - ys + h >= 0) && (py - ys < screen->h);
}

bool JudgeInScreen(int px, int py, int w, int h, int xs, int ys, int xx, int yy, int xw, int yh)
{
    return (px - xs + w >= xx) && (px - xs < xx + xw) && (py - ys + h >= yy) && (py - ys < yy + yh);
}

// ==== RLE8 图片绘制核心 ====

void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea,
    const char* Image, int Shadow, int mask, const char* colorPanel)
{
    int16_t w, h, xs, ys;
    int offset;
    uint8_t l, l1;
    uint32 pix;
    uint8_t pix1, pix2, pix3, pix4;
    uint32_t alphe;

    if (rs == 0)
        randomcount = rand() % 640;

    if (num == 0)
        offset = 0;
    else
        offset = Pidx[num - 1];

    uint8_t* pp = Ppic + offset;
    w = *reinterpret_cast<int16_t*>(pp); pp += 2;
    h = *reinterpret_cast<int16_t*>(pp); pp += 2;
    xs = *reinterpret_cast<int16_t*>(pp) + 1; pp += 2;
    ys = *reinterpret_cast<int16_t*>(pp) + 1; pp += 2;

    if ((px - xs + w < RectArea.x) && (px - xs < RectArea.x)) return;
    if ((px - xs + w > RectArea.x + RectArea.w) && (px - xs > RectArea.x + RectArea.w)) return;
    if ((py - ys + h < RectArea.y) && (py - ys < RectArea.y)) return;
    if ((py - ys + h > RectArea.y + RectArea.h) && (py - ys > RectArea.y + RectArea.h)) return;

    if (mask == 1)
    {
        for (int i1 = RectArea.x; i1 <= RectArea.x + RectArea.w; i1++)
            for (int i2 = RectArea.y; i2 <= RectArea.y + RectArea.h; i2++)
            {
                if (i1 >= 0 && i2 >= 0 && i1 < 2304 && i2 < 1152)
                    MaskArray[i1][i2] = 0;
            }
    }

    if (!JudgeInScreen(px, py, w, h, xs, ys, RectArea.x, RectArea.y, RectArea.w, RectArea.h))
        return;

    int curW, p, ix, iy;
    for (iy = 1; iy <= h; iy++)
    {
        l = *pp; pp++;
        curW = 1;
        p = 0;
        for (ix = 1; ix <= l; ix++)
        {
            l1 = *pp; pp++;
            if (p == 0)
            {
                curW += l1;
                p = 1;
            }
            else if (p == 1)
            {
                p = 2 + l1;
            }
            else if (p > 2)
            {
                p--;
                int screenX = curW - xs + px;
                int screenY = iy - ys + py;
                if (screenX >= RectArea.x && screenY >= RectArea.y &&
                    screenX < RectArea.x + RectArea.w && screenY < RectArea.y + RectArea.h)
                {
                    if ((mask != 2 && mask != 3) || MaskArray[screenX][screenY] == 1)
                    {
                        if (mask == 1)
                            MaskArray[screenX][screenY] = 1;
                        if (mask == 3)
                            MaskArray[screenX][screenY] = 0;

                        if (Image == nullptr)
                        {
                            auto* cp = reinterpret_cast<const uint8_t*>(colorPanel);
                            pix = SDL_MapSurfaceRGB(screen,
                                cp[l1 * 3] * (4 + Shadow),
                                cp[l1 * 3 + 1] * (4 + Shadow),
                                cp[l1 * 3 + 2] * (4 + Shadow));

                            if (HighLight)
                            {
                                alphe = 50;
                                pix1 = pix & 0xFF;
                                pix2 = (pix >> 8) & 0xFF;
                                pix3 = (pix >> 16) & 0xFF;
                                pix4 = (pix >> 24) & 0xFF;
                                pix1 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix1) / 100);
                                pix2 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix2) / 100);
                                pix3 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix3) / 100);
                                pix4 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix4) / 100);
                                pix = pix1 + (pix2 << 8) + (pix3 << 16) + (pix4 << 24);
                            }
                            else if (gray > 0)
                            {
                                pix1 = pix & 0xFF;
                                pix2 = (pix >> 8) & 0xFF;
                                pix3 = (pix >> 16) & 0xFF;
                                pix4 = (pix >> 24) & 0xFF;
                                uint32_t g = (pix1 * 11) / 100 + (pix2 * 59) / 100 + (pix3 * 3) / 10;
                                pix1 = static_cast<uint8_t>(((100 - gray) * pix1 + gray * g) / 100);
                                pix2 = static_cast<uint8_t>(((100 - gray) * pix2 + gray * g) / 100);
                                pix3 = static_cast<uint8_t>(((100 - gray) * pix3 + gray * g) / 100);
                                pix = pix1 + (pix1 << 8) + (pix1 << 16) + (pix4 << 24);
                            }
                            else if (blue > 0)
                            {
                                pix1 = pix & 0xFF;
                                pix2 = static_cast<uint8_t>(((pix >> 8) & 0xFF) * (150 - blue) / 150);
                                pix3 = static_cast<uint8_t>(((pix >> 16) & 0xFF) * (150 - blue) / 150);
                                pix = pix1 + (pix2 << 8) + (pix3 << 16);
                            }
                            else if (red > 0)
                            {
                                pix1 = static_cast<uint8_t>((pix & 0xFF) * (150 - red) / 150);
                                pix2 = static_cast<uint8_t>(((pix >> 8) & 0xFF) * (150 - red) / 150);
                                pix3 = (pix >> 16) & 0xFF;
                                pix = pix1 + (pix2 << 8) + (pix3 << 16);
                            }
                            else if (green > 0)
                            {
                                pix1 = static_cast<uint8_t>((pix & 0xFF) * (150 - green) / 150);
                                pix2 = (pix >> 8) & 0xFF;
                                pix3 = static_cast<uint8_t>(((pix >> 16) & 0xFF) * (150 - green) / 150);
                                pix = pix1 + (pix2 << 8) + (pix3 << 16);
                            }
                            else if (yellow > 0)
                            {
                                pix1 = static_cast<uint8_t>((pix & 0xFF) * (150 - yellow) / 150);
                                pix2 = (pix >> 8) & 0xFF;
                                pix3 = (pix >> 16) & 0xFF;
                                pix = pix1 + (pix2 << 8) + (pix3 << 16);
                            }

                            if (showblackscreen && Where == 1)
                            {
                                alphe = snowalpha[screenY][screenX];
                                if (alphe >= 100) pix = 0;
                                else if (alphe > 0)
                                {
                                    pix1 = pix & 0xFF;
                                    pix2 = (pix >> 8) & 0xFF;
                                    pix3 = (pix >> 16) & 0xFF;
                                    pix4 = (pix >> 24) & 0xFF;
                                    pix1 = static_cast<uint8_t>(((100 - alphe) * pix1) / 100);
                                    pix2 = static_cast<uint8_t>(((100 - alphe) * pix2) / 100);
                                    pix3 = static_cast<uint8_t>(((100 - alphe) * pix3) / 100);
                                    pix4 = static_cast<uint8_t>(((100 - alphe) * pix4) / 100);
                                    pix = pix1 + (pix2 << 8) + (pix3 << 16) + (pix4 << 24);
                                }
                            }

                            if (Where == 1 && Water >= 0)
                            {
                                int os = (screenY + Water / 3) % 60;
                                os = snowalpha[0][os];
                                if (os > 128) os -= 256;
                                putpixel(screen, screenX + os, screenY, pix);
                            }
                            else if (Where == 1 && rain >= 0)
                            {
                                int b = ix + randomcount;
                                if (b >= 640) b -= 640;
                                b = snowalpha[screenY][b];
                                alphe = 50;
                                if (b == 1)
                                {
                                    pix1 = pix & 0xFF;
                                    pix2 = (pix >> 8) & 0xFF;
                                    pix3 = (pix >> 16) & 0xFF;
                                    pix4 = (pix >> 24) & 0xFF;
                                    pix1 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix1) / 100);
                                    pix2 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix2) / 100);
                                    pix3 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix3) / 100);
                                    pix4 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix4) / 100);
                                    pix = pix1 + (pix2 << 8) + (pix3 << 16) + (pix4 << 24);
                                }
                                putpixel(screen, screenX, screenY, pix);
                            }
                            else if (Where == 1 && snow >= 0)
                            {
                                int b = ix + randomcount;
                                if (b >= 640) b -= 640;
                                b = snowalpha[screenY][b];
                                if (b == 1) pix = ColColor(255);
                                putpixel(screen, screenX, screenY, pix);
                            }
                            else if (Where == 1 && fog)
                            {
                                int b = ix + randomcount;
                                if (b >= 640) b -= 640;
                                alphe = snowalpha[screenY][b];
                                pix1 = pix & 0xFF;
                                pix2 = (pix >> 8) & 0xFF;
                                pix3 = (pix >> 16) & 0xFF;
                                pix4 = (pix >> 24) & 0xFF;
                                pix1 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix1) / 100);
                                pix2 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix2) / 100);
                                pix3 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix3) / 100);
                                pix4 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * pix4) / 100);
                                pix = pix1 + (pix2 << 8) + (pix3 << 16) + (pix4 << 24);
                                putpixel(screen, screenX, screenY, pix);
                            }
                            else
                            {
                                putpixel(screen, screenX, screenY, pix);
                            }
                        }
                        else
                        {
                            auto* cp = reinterpret_cast<const uint8_t*>(colorPanel);
                            *reinterpret_cast<int*>(
                                const_cast<char*>(Image) + (screenX * 1152 + screenY) * 4) =
                                SDL_MapSurfaceRGB(screen,
                                    cp[l1 * 3] * (4 + Shadow),
                                    cp[l1 * 3 + 1] * (4 + Shadow),
                                    cp[l1 * 3 + 2] * (4 + Shadow));
                        }
                    }
                }
                curW++;
                if (p == 2) p = 0;
            }
        }
    }
}

void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea,
    const char* Image, int Shadow, int mask)
{
    DrawRLE8Pic(num, px, py, Pidx, Ppic, RectArea, Image, Shadow, mask, reinterpret_cast<const char*>(ACol));
}

void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea,
    const char* Image, int Shadow)
{
    DrawRLE8Pic(num, px, py, Pidx, Ppic, RectArea, Image, Shadow, 0);
}

// ==== 坐标转换 ====

TPosition GetPositionOnScreen(int x, int y, int CenterX, int CenterY)
{
    TPosition result;
    result.x = -(x - CenterX) * 18 + (y - CenterY) * 18 + CENTER_X;
    result.y = (x - CenterX) * 9 + (y - CenterY) * 9 + CENTER_Y;
    return result;
}

// ==== Title / MPic / SPic / BPic 绘制 ====

void DrawTitlePic(int imgnum, int px, int py)
{
    int BufferIdx[101] = {};
    uint8_t BufferPic[20001] = {};

    std::string grpPath = AppPath + "resource/title.grp";
    std::string idxPath = AppPath + "resource/title.idx";

    std::ifstream idxFile(idxPath, std::ios::binary);
    std::ifstream grpFile(grpPath, std::ios::binary);
    if (!idxFile || !grpFile) return;

    idxFile.seekg(0, std::ios::end);
    auto idxLen = idxFile.tellg();
    idxFile.seekg(0);
    idxFile.read(reinterpret_cast<char*>(BufferIdx), idxLen);

    grpFile.seekg(0, std::ios::end);
    auto grpLen = grpFile.tellg();
    grpFile.seekg(0);
    grpFile.read(reinterpret_cast<char*>(BufferPic), grpLen);

    TRect Area{ 0, 0, screen->w, screen->h };
    resetpallet();
    DrawRLE8Pic(imgnum, px, py, BufferIdx, BufferPic, Area, nullptr, 0);
}

void DrawMPic(int num, int px, int py, int mask)
{
    TRect Area{ 0, 0, screen->w, screen->h };
    if (num < static_cast<int>(MIdx.size()))
    {
        if (num >= 2501 && num <= 2552)
            DrawRLE8Pic(num, px, py, MIdx.data(), MPic.data(), Area, nullptr, 0, mask, reinterpret_cast<const char*>(Col[0]));
        else
            DrawRLE8Pic(num, px, py, MIdx.data(), MPic.data(), Area, nullptr, 0, mask);
    }
}

void DrawSPic(int num, int px, int py, int x, int y, int w, int h)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, 0);
}

void DrawSPic(int num, int px, int py, int x, int y, int w, int h, int mask)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, 0, mask);
}

void InitialSPic(int num, int px, int py, int x, int y, int w, int h, int mask)
{
    if (x + w > 2303) w = 2303 - x;
    if (y + h > 1151) h = 1151 - y;
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, reinterpret_cast<const char*>(SceneImg), 0, mask);
}

void InitialSPic(int num, int px, int py, int x, int y, int w, int h)
{
    InitialSPic(num, px, py, x, y, w, h, 0);
}

void DrawBPic(int num, int px, int py, int shadow)
{
    TRect Area{ 0, 0, screen->w, screen->h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, shadow);
}

void DrawBPic(int num, int px, int py, int shadow, int mask)
{
    TRect Area{ 0, 0, screen->w, screen->h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, shadow, mask);
}

void DrawBPic(int num, int x, int y, int w, int h, int px, int py, int shadow)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, shadow);
}

void DrawBPic(int num, int x, int y, int w, int h, int px, int py, int shadow, int mask)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, shadow, mask);
}

void DrawBRolePic(int num, int px, int py, int shadow, int mask)
{
    TRect Area{ 0, 0, screen->w, screen->h };
    if (num < static_cast<int>(WIdx.size()))
        DrawRLE8Pic(num, px, py, WIdx.data(), WPic.data(), Area, nullptr, shadow, mask);
}

void DrawBRolePic(int num, int x, int y, int w, int h, int px, int py, int shadow, int mask)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(WIdx.size()))
        DrawRLE8Pic(num, px, py, WIdx.data(), WPic.data(), Area, nullptr, shadow, mask);
}

void DrawBPicInRect(int num, int px, int py, int shadow, int x, int y, int w, int h)
{
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, nullptr, shadow);
}

void InitialBPic(int num, int px, int py)
{
    TRect Area{ 0, 0, 2304, 1152 };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, reinterpret_cast<const char*>(BFieldImg), 0);
}

void InitialBPic(int num, int px, int py, int x, int y, int w, int h, int mask)
{
    if (x + w > 2303) w = 2303 - x;
    if (y + h > 1151) h = 1151 - y;
    TRect Area{ x, y, w, h };
    if (num < static_cast<int>(SIdx.size()))
        DrawRLE8Pic(num, px, py, SIdx.data(), SPic.data(), Area, reinterpret_cast<const char*>(BFieldImg), 0, mask);
}

// ==== 文字编码转换 ====

std::wstring Big5ToUnicode(const char* str)
{
#ifdef _WIN32
    int len = MultiByteToWideChar(950, 0, str, -1, nullptr, 0);
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(950, 0, str, static_cast<int>(strlen(str)), result.data(), len + 1);
    return L" " + result;
#else
    return L" ";
#endif
}

std::wstring GBKToUnicode(const char* str)
{
#ifdef _WIN32
    int len = MultiByteToWideChar(936, 0, str, -1, nullptr, 0);
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(936, 0, str, static_cast<int>(strlen(str)), result.data(), len + 1);
    return L" " + result;
#else
    return L" ";
#endif
}

std::string UnicodeToBig5(const wchar_t* str)
{
#ifdef _WIN32
    int len = WideCharToMultiByte(950, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len + 1, 0);
    WideCharToMultiByte(950, 0, str, -1, result.data(), len + 1, nullptr, nullptr);
    return result;
#else
    return "";
#endif
}

std::string UnicodeToGBK(const wchar_t* str)
{
#ifdef _WIN32
    int len = WideCharToMultiByte(936, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len + 1, 0);
    WideCharToMultiByte(936, 0, str, -1, result.data(), len + 1, nullptr, nullptr);
    return result;
#else
    return "";
#endif
}

// ==== 文字绘制 ====

void DrawText_(SDL_Surface* sur, uint16_t* word, int x_pos, int y_pos, uint32 color)
{
    SDL_Rect dest;
    uint16_t pword[3] = { 32, 0, 0 };

    uint8_t c3 = color & 0xFF;
    uint8_t c2 = (color >> 8) & 0xFF;
    uint8_t c1 = (color >> 16) & 0xFF;
    uint8_t c4 = (color >> 24) & 0xFF;
    color = c1 + (c2 << 8) + (c3 << 16) + (c4 << 24);
    SDL_Color tempcolor;
    std::memcpy(&tempcolor, &color, sizeof(uint32));

    std::wstring t;
    if (Simple == 1)
    {
        t = Traditional2Simplified(reinterpret_cast<wchar_t*>(word));
        word = reinterpret_cast<uint16_t*>(t.data());
    }

    int x = x_pos;
    dest.x = x_pos;
    while (*word > 0)
    {
        pword[1] = *word;
        word++;
        if (pword[1] > 128)
        {
            TextSurface = TTF_RenderGlyph_Blended(Font, pword[1], tempcolor);
            dest.x = x_pos + 10;
            dest.y = y_pos;
            SDL_BlitSurface(TextSurface, nullptr, sur, &dest);
            x_pos += 20;
        }
        else
        {
            if (pword[1] == 42) // '*' 换行
            {
                pword[1] = 0;
                x_pos = x;
                y_pos += 19;
            }
            TextSurface = TTF_RenderGlyph_Blended(EngFont, pword[1], tempcolor);
            dest.x = x_pos + 10;
            dest.y = y_pos + 4;
            SDL_BlitSurface(TextSurface, nullptr, sur, &dest);
            x_pos += 10;
        }
        SDL_DestroySurface(TextSurface);
    }
}

void DrawEngText(SDL_Surface* sur, uint16_t* word, int x_pos, int y_pos, uint32 color)
{
    uint8_t c3 = color & 0xFF;
    uint8_t c2 = (color >> 8) & 0xFF;
    uint8_t c1 = (color >> 16) & 0xFF;
    uint8_t c4 = (color >> 24) & 0xFF;
    color = c1 + (c2 << 8) + (c3 << 16) + (c4 << 24);
    SDL_Color tempcolor;
    std::memcpy(&tempcolor, &color, sizeof(uint32));

    std::string ustr;
    auto* p = word;
    while (*p != 0)
    {
        if (*p < 0x80)
            ustr += static_cast<char>(*p);
        else if (*p < 0x800)
        {
            ustr += static_cast<char>(0xC0 | (*p >> 6));
            ustr += static_cast<char>(0x80 | (*p & 0x3F));
        }
        else
        {
            ustr += static_cast<char>(0xE0 | (*p >> 12));
            ustr += static_cast<char>(0x80 | ((*p >> 6) & 0x3F));
            ustr += static_cast<char>(0x80 | (*p & 0x3F));
        }
        p++;
    }

    TextSurface = TTF_RenderText_Blended(EngFont, ustr.c_str(), 0, tempcolor);
    SDL_Rect dest{ x_pos, y_pos + 4, 0, 0 };
    SDL_BlitSurface(TextSurface, nullptr, sur, &dest);
    SDL_DestroySurface(TextSurface);
}

void DrawShadowText(uint16_t* word, int x_pos, int y_pos, uint32 color1, uint32 color2)
{
    DrawText_(screen, word, x_pos + 1, y_pos, color2);
    DrawText_(screen, word, x_pos, y_pos, color1);
}

void DrawEngShadowText(uint16_t* word, int x_pos, int y_pos, uint32 color1, uint32 color2)
{
    DrawEngText(screen, word, x_pos + 1, y_pos, color2);
    DrawEngText(screen, word, x_pos, y_pos, color1);
}

void DrawBig5Text(SDL_Surface* sur, const char* str, int x_pos, int y_pos, uint32 color)
{
    std::wstring words = Big5ToUnicode(str);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos, y_pos, color);
}

void DrawBig5ShadowText(const char* word, int x_pos, int y_pos, uint32 color1, uint32 color2)
{
    std::wstring words = Big5ToUnicode(word);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos + 1, y_pos, color2);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos, y_pos, color1);
}

void DrawGBKText(SDL_Surface* sur, const char* str, int x_pos, int y_pos, uint32 color)
{
    std::wstring words = GBKToUnicode(str);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos, y_pos, color);
}

void DrawGBKShadowText(const char* word, int x_pos, int y_pos, uint32 color1, uint32 color2)
{
    std::wstring words = GBKToUnicode(word);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos + 1, y_pos, color2);
    DrawText_(screen, reinterpret_cast<uint16_t*>(words.data()), x_pos, y_pos, color1);
}

void DrawTextWithRect(uint16_t* word, int x, int y, int w, uint32 color1, uint32 color2)
{
    DrawRectangle(x, y, w, 28, 0, ColColor(0, 255), 30);
    DrawShadowText(word, x - 17, y + 2, color1, color2);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

// ==== 画线 ====

void DrawLine(int x1, int y1, int x2, int y2, int color, int Width)
{
    if (x1 > x2)
    {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }
    int x = x2 - x1 - Width;
    int y = y2 - y1 - Width;
    int p;
    if (x > 0)
    {
        for (int i = 0; i < x; i++)
        {
            p = (y * i) / x;
            DrawRectangleWithoutFrame(x1 + i, y1 + p, Width, Width, color, 100);
        }
    }
    else if (y > 0)
    {
        for (int i = 0; i < y; i++)
        {
            p = (x * i) / y;
            DrawRectangleWithoutFrame(x1 + i, y1 + p, Width, Width, color, 100);
        }
    }
    else
    {
        DrawRectangleWithoutFrame(x1, y1, Width, Width, color, 100);
    }
}

// ==== 矩形绘制 ====

void DrawRectangle(int x, int y, int w, int h, uint32 colorin, uint32 colorframe, int alpha)
{
    w = abs(w);
    h = abs(h);
    auto* tempscr = SDL_CreateSurface(w + 1, h + 1,
        SDL_GetPixelFormatForMasks(32, RMask, GMask, BMask, AMask));
    if (!tempscr) return;

    uint8_t r, g, b, r1, g1, b1;
    SDL_GetRGB(colorin, SDL_GetPixelFormatDetails(tempscr->format), SDL_GetSurfacePalette(tempscr), &r, &g, &b);
    SDL_GetRGB(colorframe, SDL_GetPixelFormatDetails(tempscr->format), SDL_GetSurfacePalette(tempscr), &r1, &g1, &b1);
    SDL_FillSurfaceRect(tempscr, nullptr, SDL_MapSurfaceRGBA(tempscr, r, g, b, static_cast<uint8_t>(alpha * 255 / 100)));

    for (int i1 = 0; i1 <= w; i1++)
    {
        for (int i2 = 0; i2 <= h; i2++)
        {
            int l1 = i1 + i2;
            int l2 = -(i1 - w) + i2;
            int l3 = i1 - (i2 - h);
            int l4 = -(i1 - w) - (i2 - h);
            if (!(l1 >= 4 && l2 >= 4 && l3 >= 4 && l4 >= 4))
            {
                putpixel(tempscr, i1, i2, 0);
            }
            if ((l1 >= 4 && l2 >= 4 && l3 >= 4 && l4 >= 4 &&
                (i1 == 0 || i1 == w || i2 == 0 || i2 == h)) ||
                l1 == 4 || l2 == 4 || l3 == 4 || l4 == 4)
            {
                auto a = static_cast<uint8_t>(std::clamp(
                    static_cast<int>(250 - std::abs(static_cast<double>(i1) / w + static_cast<double>(i2) / h - 1.0) * 150),
                    0, 255));
                putpixel(tempscr, i1, i2, SDL_MapSurfaceRGBA(tempscr, r1, g1, b1, a));
            }
        }
    }

    SDL_Rect dest{ x, y, 0, 0 };
    SDL_BlitSurface(tempscr, nullptr, screen, &dest);
    SDL_DestroySurface(tempscr);
}

void DrawRectangleWithoutFrame(int x, int y, int w, int h, uint32 colorin, int alphe)
{
    uint8_t color1 = colorin & 0xFF;
    uint8_t color2 = (colorin >> 8) & 0xFF;
    uint8_t color3 = (colorin >> 16) & 0xFF;
    uint8_t color4 = (colorin >> 24) & 0xFF;
    for (int i1 = x; i1 <= x + w; i1++)
    {
        for (int i2 = y; i2 <= y + h; i2++)
        {
            uint32 pix = getpixel(screen, i1, i2);
            uint8_t pix1 = static_cast<uint8_t>((alphe * color1 + (100 - alphe) * (pix & 0xFF)) / 100);
            uint8_t pix2 = static_cast<uint8_t>((alphe * color2 + (100 - alphe) * ((pix >> 8) & 0xFF)) / 100);
            uint8_t pix3 = static_cast<uint8_t>((alphe * color3 + (100 - alphe) * ((pix >> 16) & 0xFF)) / 100);
            uint8_t pix4 = static_cast<uint8_t>((alphe * color4 + (100 - alphe) * ((pix >> 24) & 0xFF)) / 100);
            putpixel(screen, i1, i2, pix1 + (pix2 << 8) + (pix3 << 16) + (pix4 << 24));
        }
    }
}

// ==== Redraw ====

void Redraw()
{
    switch (Where)
    {
    case 0: DrawMMap(); break;
    case 1: DrawScene(); break;
    case 2: DrawWholeBField(); break;
    case 3:
        display_imgFromSurface(BEGIN_PIC.pic, 0, 0);
        DrawGBKShadowText(versionstr.c_str(), 5, CENTER_Y * 2 - 30, ColColor(0x64), ColColor(0x66));
        break;
    case 4:
        display_imgFromSurface(DEATH_PIC.pic, 0, 0);
        break;
    }
}

// ==== SDL_UpdateRect2 / SDL_GetMouseState2 ====

void SDL_UpdateRect2(SDL_Surface* surf, int x, int y, int w, int h)
{
    if (render == nullptr || screenTex == nullptr) return;
    DrawVirtualKey();
    if (virtualKeyScr != nullptr)
        SDL_BlitSurface(virtualKeyScr, nullptr, surf, nullptr);
    SDL_UpdateTexture(screenTex, nullptr, surf->pixels, surf->pitch);

    if (CellPhone == 1 && KEEP_SCREEN_RATIO == 1)
    {
        auto info = KeepRatioScale(surf->w, surf->h, RESOLUTIONX, RESOLUTIONY);
        SDL_FRect dstRect;
        dstRect.x = static_cast<float>(info.px);
        dstRect.y = static_cast<float>(info.py);
        dstRect.w = static_cast<float>(surf->w * info.num / info.den);
        dstRect.h = static_cast<float>(surf->h * info.num / info.den);
        SDL_RenderClear(render);
        SDL_RenderTexture(render, screenTex, nullptr, &dstRect);
    }
    else
    {
        SDL_RenderTexture(render, screenTex, nullptr, nullptr);
    }
    SDL_RenderPresent(render);
}

void SDL_GetMouseState2(int& x, int& y)
{
    float fx, fy;
    SDL_GetMouseState(&fx, &fy);
    x = static_cast<int>(fx);
    y = static_cast<int>(fy);

    if (CellPhone == 1 && KEEP_SCREEN_RATIO == 1 && screen != nullptr)
    {
        auto info = KeepRatioScale(screen->w, screen->h, RESOLUTIONX, RESOLUTIONY);
        if (info.num > 0)
        {
            x = (x - info.px) * info.den / info.num;
            y = (y - info.py) * info.den / info.num;
        }
    }
}

void ResizeWindow(int w, int h)
{
    SDL_SetWindowSize(window, w, h);
}

void SwitchFullscreen()
{
    SDL_SetWindowFullscreen(window, fullscreen != 0);
}

bool InRegion(int x, int x0, int x1)
{
    return x >= x0 && x <= x1;
}

void kyslog(const std::string& formatstring, bool cr)
{
    std::cout << formatstring;
    if (cr) std::cout << '\n';
}

bool MouseInRegion(int x, int y, int w, int h)
{
    int mx, my;
    SDL_GetMouseState2(mx, my);
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

bool MouseInRegion(int x, int y, int w, int h, int& x1, int& y1)
{
    SDL_GetMouseState2(x1, y1);
    return x1 >= x && x1 <= x + w && y1 >= y && y1 <= y + h;
}

SDL_FRect GetRealRect(int& x, int& y, int& w, int& h, int force)
{
    SDL_FRect rect;
    rect.x = static_cast<float>(x);
    rect.y = static_cast<float>(y);
    rect.w = static_cast<float>(w);
    rect.h = static_cast<float>(h);
    return rect;
}

SDL_FRect GetRealRect(SDL_FRect rect, int force)
{
    return rect;
}

TStretchInfo KeepRatioScale(int w1, int h1, int w2, int h2)
{
    TStretchInfo info{};
    if (w1 == 0 || h1 == 0 || w2 == 0 || h2 == 0)
    {
        info.num = 1; info.den = 1;
        return info;
    }
    if (w2 * h1 > w1 * h2)
    {
        info.num = h2; info.den = h1;
        info.px = (w2 - w1 * h2 / h1) / 2;
        info.py = 0;
    }
    else
    {
        info.num = w2; info.den = w1;
        info.px = 0;
        info.py = (h2 - h1 * w2 / w1) / 2;
    }
    return info;
}

void DrawVirtualKey()
{
    if (ShowVirtualKey == 0 || CellPhone == 0) return;
    if (virtualKeyScr == nullptr) return;

    SDL_FillSurfaceRect(virtualKeyScr, nullptr, SDL_MapSurfaceRGBA(virtualKeyScr, 0, 0, 0, 0));

    SDL_Rect rect{ 0, 0, 0, 0 };
    rect.x = VirtualKeyX;
    rect.y = VirtualKeyY;
    if (VirtualKeyU != nullptr) SDL_BlitSurface(VirtualKeyU, nullptr, virtualKeyScr, &rect);

    rect.x = VirtualKeyX - VirtualKeySize;
    rect.y = VirtualKeyY + VirtualKeySize;
    if (VirtualKeyL != nullptr) SDL_BlitSurface(VirtualKeyL, nullptr, virtualKeyScr, &rect);

    rect.x = VirtualKeyX;
    rect.y = VirtualKeyY + VirtualKeySize * 2;
    if (VirtualKeyD != nullptr) SDL_BlitSurface(VirtualKeyD, nullptr, virtualKeyScr, &rect);

    rect.x = VirtualKeyX + VirtualKeySize;
    rect.y = VirtualKeyY + VirtualKeySize;
    if (VirtualKeyR != nullptr) SDL_BlitSurface(VirtualKeyR, nullptr, virtualKeyScr, &rect);

    rect.x = 0;
    rect.y = 0;
    if (VirtualKeyB != nullptr) SDL_BlitSurface(VirtualKeyB, nullptr, virtualKeyScr, &rect);

    rect.x = virtualKeyScr->w - 100;
    rect.y = virtualKeyScr->h - 100;
    if (VirtualKeyA != nullptr) SDL_BlitSurface(VirtualKeyA, nullptr, virtualKeyScr, &rect);
}

uint32 CheckBasicEvent()
{
    switch (event.type)
    {
    case SDL_EVENT_QUIT:
        QuitConfirm();
        return SDLK_ESCAPE;
    case SDL_EVENT_WINDOW_RESIZED:
        ResizeWindow(event.window.data1, event.window.data2);
        return 0;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        PlayMP3(nowmusic, -1, 0);
        return 0;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        StopMP3();
        return 0;
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
        if (event.key.key == SDLK_KP_ENTER)
            event.key.key = SDLK_RETURN;
        return event.key.key;
    default:
        return 0;
    }
}

void QuitConfirm()
{
    if (AskingQuit) return;
    AskingQuit = true;

    SDL_Surface* temp = SDL_ConvertSurface(screen, screen->format);
    if (temp == nullptr)
    {
        AskingQuit = false;
        return;
    }

    DrawRectangleWithoutFrame(0, 0, screen->w, screen->h, 0, 50);
    MenuString.clear();
    MenuString.push_back(L" 取消");
    MenuString.push_back(L" 確認");
    if (CommonMenu(CENTER_X * 2 - 50, 2, 45, 1) == 1)
    {
        SDL_DestroySurface(temp);
        AskingQuit = false;
        Quit();
        return;
    }

    SDL_BlitSurface(temp, nullptr, screen, nullptr);
    SDL_DestroySurface(temp);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    AskingQuit = false;
}

void resetpallet()
{
    int paletteIndex = 0;
    if (Where == 1 && CurScene >= 0 && CurScene < static_cast<int>(RScene.size()))
    {
        if (RScene[CurScene].Pallet >= 0 && RScene[CurScene].Pallet < 4)
            paletteIndex = RScene[CurScene].Pallet;
    }
    std::memcpy(ACol, Col[paletteIndex], 768);
}

void resetpallet(int num)
{
    if (num >= 0 && num < 4)
        std::memcpy(ACol, Col[num], 768);
}

uint16 RoRforUInt16(uint16 a, uint16 n)
{
    n &= 15;
    return static_cast<uint16>((a >> n) | (a << (16 - n)));
}

uint16 RoLforUint16(uint16 a, uint16 n)
{
    n &= 15;
    return static_cast<uint16>((a << n) | (a >> (16 - n)));
}

uint8_t RoRforByte(uint8_t a, uint16 n)
{
    n &= 7;
    return static_cast<uint8_t>((a >> n) | (a << (8 - n)));
}

uint8_t RoLforByte(uint8_t a, uint16 n)
{
    n &= 7;
    return static_cast<uint8_t>((a << n) | (a >> (8 - n)));
}

// ==== 场景绘制 ====

void DrawSNewPic(int num, int px, int py, int x, int y, int w, int h, int mask)
{
    if (num < 3 || num >= static_cast<int>(ScenePic.size())) return;
    int bpp = 4;
    int x1 = px - ScenePic[num].x + 1;
    int y1 = py - ScenePic[num].y + 1;
    if (x1 + ScenePic[num].pic->w < x && x1 < x) return;
    if (x1 + ScenePic[num].pic->w > x + w && x1 > x + w) return;
    if (y1 + ScenePic[num].pic->h < y && y1 < y) return;
    if (y1 + ScenePic[num].pic->h > y + h && y1 > y + h) return;
    if (mask == 1)
        for (int i1 = x; i1 <= x + w; i1++)
            for (int i2 = y; i2 <= y + h; i2++)
                if (i1 >= 0 && i2 >= 0 && i1 < 2304 && i2 < 1152)
                    MaskArray[i1][i2] = 0;
    for (int i1 = 0; i1 < ScenePic[num].pic->w; i1++)
        for (int i2 = 0; i2 < ScenePic[num].pic->h; i2++)
        {
            int sx = x1 + i1, sy = y1 + i2;
            if (sx < x || sx > x + w || sy < y || sy > y + h) continue;
            if (mask > 1 && MaskArray[sx][sy] != 1) continue;
            auto* pp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(ScenePic[num].pic->pixels) + i2 * ScenePic[num].pic->pitch + i1 * bpp);
            uint32 c = *pp;
            uint32 alpha = (c >> 24) & 0xFF;
            uint8_t col1 = (c >> 16) & 0xFF;
            uint8_t col2 = (c >> 8) & 0xFF;
            uint8_t col3 = c & 0xFF;
            ApplyScenePalette(col1, col2, col3);
            auto* sp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(screen->pixels) + sy * screen->pitch + sx * bpp);
            uint32 pix = *sp;
            uint8_t pix1 = (pix >> 16) & 0xFF;
            uint8_t pix2 = (pix >> 8) & 0xFF;
            uint8_t pix3 = pix & 0xFF;
            if (alpha == 0 && mask == 1) MaskArray[sx][sy] = 1;
            BlendRGB255(pix1, pix2, pix3, col1, col2, col3, alpha);
            if (mask == 1) MaskArray[sx][sy] = 1;
            if (mask == 3) MaskArray[sx][sy] = 0;
            if (HighLight)
            {
                int a2 = 50;
                pix1 = static_cast<uint8_t>((a2 * 0xFF + (100 - a2) * pix1) / 100);
                pix2 = static_cast<uint8_t>((a2 * 0xFF + (100 - a2) * pix2) / 100);
                pix3 = static_cast<uint8_t>((a2 * 0xFF + (100 - a2) * pix3) / 100);
            }
            uint32 out = (pix1 << 16) | (pix2 << 8) | pix3 | AMask;
            *sp = out;
        }
}

void InitNewPic(int num, int px, int py, int x, int y, int w, int h) { InitNewPic(num, px, py, x, y, w, h, 0); }

void InitNewPic(int num, int px, int py, int x, int y, int w, int h, int mask)
{
    if (num < 3 || num >= static_cast<int>(ScenePic.size())) return;
    int bpp = 4;
    int x1 = px - ScenePic[num].x + 1;
    int y1 = py - ScenePic[num].y + 1;
    if (x1 + ScenePic[num].pic->w < x && x1 < x) return;
    if (x1 + ScenePic[num].pic->w > x + w && x1 > x + w) return;
    if (y1 + ScenePic[num].pic->h < y && y1 < y) return;
    if (y1 + ScenePic[num].pic->h > y + h && y1 > y + h) return;
    for (int i1 = 0; i1 < ScenePic[num].pic->w; i1++)
        for (int i2 = 0; i2 < ScenePic[num].pic->h; i2++)
        {
            int sx = x1 + i1, sy = y1 + i2;
            if (mask == 1) MaskArray[sx][sy] = 0;
            if (sx < x || sx > x + w || sy < y || sy > y + h) continue;
            if (mask >= 2 && MaskArray[sx][sy] != 1) continue;
            auto* pp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(ScenePic[num].pic->pixels) + i2 * ScenePic[num].pic->pitch + i1 * bpp);
            uint32 c = *pp;
            if ((c & 0xFF000000) != 0)
            {
                if (mask == 1) { MaskArray[sx][sy] = 1; SceneImg[sx][sy] = 0; continue; }
                uint32 pix = SceneImg[sx][sy];
                uint8_t pix1 = (pix >> 16) & 0xFF;
                uint8_t pix2 = (pix >> 8) & 0xFF;
                uint8_t pix3 = pix & 0xFF;
                int alpha = (c >> 24) & 0xFF;
                uint8_t col1 = (c >> 16) & 0xFF;
                uint8_t col2 = (c >> 8) & 0xFF;
                uint8_t col3 = c & 0xFF;
                ApplyScenePalette(col1, col2, col3);
                BlendRGB255(pix1, pix2, pix3, col1, col2, col3, alpha);
                SceneImg[sx][sy] = (pix1 << 16) | (pix2 << 8) | pix3 | AMask;
            }
        }
}

void DrawHeadPic(int num, int px, int py)
{
    if (num < 0 || num >= static_cast<int>(Head_Pic.size())) return;
    if (Head_Pic[num].pic == nullptr) return;
    DrawRectangle(px, py - 57, 57, 59, 0, ColColor(255), 0);
    int bpp = 4;
    int x1 = px - Head_Pic[num].x + 1;
    int y1 = py - Head_Pic[num].y + 1;
    for (int i1 = 0; i1 < Head_Pic[num].pic->w; i1++)
        for (int i2 = 0; i2 < Head_Pic[num].pic->h; i2++)
        {
            int sx = x1 + i1, sy = y1 + i2;
            if (sx < 0 || sx >= screen->w || sy < 0 || sy >= screen->h) continue;
            auto* pp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(Head_Pic[num].pic->pixels) + i2 * Head_Pic[num].pic->pitch + i1 * bpp);
            uint32 c = *pp;
            auto* sp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(screen->pixels) + sy * screen->pitch + sx * bpp);
            uint32 pix = *sp;
            uint8_t pix1 = pix & 0xFF, pix2 = (pix >> 8) & 0xFF, pix3 = (pix >> 16) & 0xFF;
            int alpha = (c >> 24) & 0xFF;
            uint8_t col1 = c & 0xFF, col2 = (c >> 8) & 0xFF, col3 = (c >> 16) & 0xFF;
            if (gray > 0)
            {
                uint32 g = (col1 * 11) / 100 + (col2 * 59) / 100 + (col3 * 3) / 10;
                col1 = static_cast<uint8_t>(((100 - gray) * col1 + gray * g) / 100);
                col2 = static_cast<uint8_t>(((100 - gray) * col2 + gray * g) / 100);
                col3 = static_cast<uint8_t>(((100 - gray) * col3 + gray * g) / 100);
            }
            pix1 = static_cast<uint8_t>((alpha * col1 + (255 - alpha) * pix1) / 255);
            pix2 = static_cast<uint8_t>((alpha * col2 + (255 - alpha) * pix2) / 255);
            pix3 = static_cast<uint8_t>((alpha * col3 + (255 - alpha) * pix3) / 255);
            *sp = pix1 | (pix2 << 8) | (pix3 << 16) | (0xFFu << 24);
        }
}

void DrawMMap()
{
    for (int i1 = 0; i1 <= screen->w; i1++)
        for (int i2 = 0; i2 <= screen->h; i2++)
            MaskArray[i1][i2] = 1;

    struct BldEntry { int x, y, cx, cy; };
    std::vector<BldEntry> blist;
    int16_t temp[480][480] = {};

    for (int sum = -29; sum <= 41; sum++)
        for (int i = -16; i <= 16; i++)
        {
            int i1 = Mx + i + (sum / 2);
            int i2 = My - i + (sum - sum / 2);
            if (i1 >= 0 && i1 < 480 && i2 >= 0 && i2 < 480)
            {
                temp[i1][i2] = 0;
                if (Building[i1][i2] != 0) temp[i1][i2] = Building[i1][i2];
                if (i1 == Mx && i2 == My)
                {
                    if (InShip == 0)
                    {
                        if (Still == 0)
                            temp[i1][i2] = static_cast<int16_t>((2501 + MFace * 7 + MStep) * 2);
                        else
                            temp[i1][i2] = static_cast<int16_t>((2528 + MFace * 6 + MStep) * 2);
                    }
                    else
                        temp[i1][i2] = static_cast<int16_t>((3714 + MFace * 4 + (MStep + 1) / 2) * 2);
                }
                if (i1 == ShipY && i2 == ShipX && InShip == 0)
                    temp[i1][i2] = static_cast<int16_t>((3715 + ShipFace * 4) * 2);
                if (temp[i1][i2] > 0)
                {
                    int16_t w_ = *reinterpret_cast<int16_t*>(&MPic[MIdx[temp[i1][i2] / 2 - 1]]);
                    blist.push_back({i1, i2, i1 * 2 - (w_ + 35) / 36 + 1, i2 * 2 - (w_ + 35) / 36 + 1});
                }
            }
            else
            {
                TPosition pos = GetPositionOnScreen(i1, i2, Mx, My);
                DrawMPic(0, pos.x, pos.y, 3);
            }
        }

    // Sort by center sum for correct draw order
    for (size_t a = 0; a + 1 < blist.size(); a++)
        for (size_t b = a + 1; b < blist.size(); b++)
            if (blist[a].cx + blist[a].cy > blist[b].cx + blist[b].cy)
                std::swap(blist[a], blist[b]);
    for (int i = static_cast<int>(blist.size()) - 1; i >= 0; i--)
    {
        TPosition pos = GetPositionOnScreen(blist[i].x, blist[i].y, Mx, My);
        DrawMPic(temp[blist[i].x][blist[i].y] / 2, pos.x, pos.y, 3);
    }

    for (int sum = 41; sum >= -29; sum--)
        for (int i = 16; i >= -16; i--)
        {
            int i1 = Mx + i + (sum / 2);
            int i2 = My - i + (sum - sum / 2);
            TPosition pos = GetPositionOnScreen(i1, i2, Mx, My);
            if (i1 >= 0 && i1 < 480 && i2 >= 0 && i2 < 480)
            {
                if (sum >= -27 && sum <= 28 && i >= -11 && i <= 11)
                {
                    if (Surface[i1][i2] > 0)
                        DrawMPic(Surface[i1][i2] / 2, pos.x, pos.y, 3);
                    DrawMPic(Earth[i1][i2] / 2, pos.x, pos.y, 3);
                }
            }
        }
    DrawClouds();
}

void DrawScene()
{
    if (CurEvent < 0)
    {
        DrawSceneWithoutRole(Sx, Sy);
        DrawRoleOnScene(Sx, Sy);
    }
    else
    {
        DrawSceneWithoutRole(Cx, Cy);
        DrawRoleOnScene(Cx, Cy);
    }
    if (time_ > 0)
    {
        std::wstring word = std::to_wstring(time_ / 60) + L":" + (time_ % 60 < 10 ? L"0" : L"") + std::to_wstring(time_ % 60);
        DrawShadowText(reinterpret_cast<uint16_t*>(word.data()), 5, 5, ColColor(0, 5), ColColor(0, 7));
    }
}

void DrawSceneWithoutRole(int x, int y)
{
    LoadScenePart(-x * 18 + y * 18 + 1151 - CENTER_X, x * 9 + y * 9 + 9 - CENTER_Y);
}

void DrawRoleOnScene(int x, int y)
{
    if (!ShowMR) return;
    TPosition pos = GetPositionOnScreen(Sx, Sy, x, y);
    DrawSPic(2501 + SFace * 7 + SStep, pos.x, pos.y - SData[CurScene][4][Sx][Sy],
        pos.x - 20, pos.y - 60 - SData[CurScene][4][Sx][Sy], 40, 60, 1);

    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            TPosition pos1 = GetPositionOnScreen(i1, i2, x, y);
            int16_t s0 = SData[CurScene][0][i1][i2];
            int16_t s1 = SData[CurScene][1][i1][i2];
            int16_t s2 = SData[CurScene][2][i1][i2];
            int16_t s3 = SData[CurScene][3][i1][i2];
            int16_t h4 = SData[CurScene][4][i1][i2];
            int16_t h5 = SData[CurScene][5][i1][i2];
            int ph = SData[CurScene][4][Sx][Sy];

            if (s0 > 0) DrawSPic(s0 / 2, pos1.x, pos1.y, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);
            else if (s0 < 0) DrawSNewPic(-s0 / 2, pos1.x, pos1.y, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);

            if (s1 > 0) DrawSPic(s1 / 2, pos1.x, pos1.y - h4, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);
            else if (s1 < 0) DrawSNewPic(-s1 / 2, pos1.x, pos1.y - h4, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);

            if (i1 == Sx && i2 == Sy)
                DrawSPic(2501 + SFace * 7 + SStep, pos1.x, pos1.y - h4, pos.x - 20, pos.y - 60 - ph, 40, 60, 1);

            if (s2 > 0) DrawSPic(s2 / 2, pos1.x, pos1.y - h5, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);
            else if (s2 < 0) DrawSNewPic(-s2 / 2, pos1.x, pos1.y - h5, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);

            if (s3 >= 0)
            {
                int16_t dpic = DData[CurScene][s3][5];
                if (dpic > 0) DrawSPic(dpic / 2, pos1.x, pos1.y - h4, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);
                if (dpic < 0) DrawSNewPic(-dpic / 2, pos1.x, pos1.y - h4, pos.x - 20, pos.y - 60 - ph, 40, 60, 2);
            }
        }
}

void InitialScene()
{
    for (int i1 = 0; i1 < 2304; i1++)
        for (int i2 = 0; i2 < 1152; i2++)
            SceneImg[i1][i2] = 0;
    SetScene();
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            int x = -i1 * 18 + i2 * 18 + 1151;
            int y = i1 * 9 + i2 * 9 + 9;
            if (SData[CurScene][0][i1][i2] > 0)
                InitialSPic(SData[CurScene][0][i1][i2] / 2, x, y, 0, 0, 2304, 1152);
            else if (SData[CurScene][0][i1][i2] < 0)
                InitNewPic(-SData[CurScene][0][i1][i2] / 2, x, y, 0, 0, 2304, 1152);
            if (SData[CurScene][1][i1][i2] > 0)
                InitialSPic(SData[CurScene][1][i1][i2] / 2, x, y - SData[CurScene][4][i1][i2], 0, 0, 2304, 1152);
            else if (SData[CurScene][1][i1][i2] < 0)
                InitNewPic(-SData[CurScene][1][i1][i2] / 2, x, y - SData[CurScene][4][i1][i2], 0, 0, 2304, 1152);
            if (SData[CurScene][2][i1][i2] > 0)
                InitialSPic(SData[CurScene][2][i1][i2] / 2, x, y - SData[CurScene][5][i1][i2], 0, 0, 2304, 1152);
            else if (SData[CurScene][2][i1][i2] < 0)
                InitNewPic(-SData[CurScene][2][i1][i2] / 2, x, y - SData[CurScene][5][i1][i2], 0, 0, 2304, 1152);
            if (SData[CurScene][3][i1][i2] >= 0)
            {
                int ev = SData[CurScene][3][i1][i2];
                if (DData[CurScene][ev][7] > 0) DData[CurScene][ev][5] = DData[CurScene][ev][7];
                if (DData[CurScene][ev][5] > 0)
                    InitialSPic(DData[CurScene][ev][5] / 2, x, y - SData[CurScene][4][i1][i2], 0, 0, 2304, 1152);
                if (DData[CurScene][ev][7] < 0) DData[CurScene][ev][5] = DData[CurScene][ev][7];
                if (DData[CurScene][ev][5] < 0)
                    InitNewPic(-DData[CurScene][ev][5] / 2, x, y - SData[CurScene][4][i1][i2], 0, 0, 2304, 1152);
            }
        }
}

void UpdateScene(int xs, int ys, int oldpic, int newpic)
{
    if (xs < 0 || ys < 0 || xs >= 64 || ys >= 64) return;
    int x = -xs * 18 + ys * 18 + 1151;
    int y = xs * 9 + ys * 9 + 9;
    oldpic /= 2; newpic /= 2;
    int16_t xp1, yp1, w1, h1, xp2, yp2, w2, h2;
    auto getRect = [&](int pic, int16_t& xp, int16_t& yp, int16_t& pw, int16_t& ph_) {
        if (pic > 0) {
            int offset = SIdx[pic - 1];
            xp = static_cast<int16_t>(x - *reinterpret_cast<int16_t*>(&SPic[offset + 4]));
            yp = static_cast<int16_t>(y - *reinterpret_cast<int16_t*>(&SPic[offset + 6]) - SData[CurScene][4][xs][ys]);
            pw = *reinterpret_cast<int16_t*>(&SPic[offset]);
            ph_ = *reinterpret_cast<int16_t*>(&SPic[offset + 2]);
        } else if (pic < -1 && -pic < static_cast<int>(ScenePic.size())) {
            xp = static_cast<int16_t>(x - ScenePic[-pic].x);
            yp = static_cast<int16_t>(y - ScenePic[-pic].y - SData[CurScene][4][xs][ys]);
            pw = static_cast<int16_t>(ScenePic[-pic].pic->w);
            ph_ = static_cast<int16_t>(ScenePic[-pic].pic->h);
        } else { xp = static_cast<int16_t>(x); yp = static_cast<int16_t>(y - SData[CurScene][4][xs][ys]); pw = 0; ph_ = 0; }
    };
    getRect(oldpic, xp1, yp1, w1, h1);
    getRect(newpic, xp2, yp2, w2, h2);
    int16_t xp = std::min(xp2, xp1) - 1;
    int16_t yp = std::min(yp2, yp1) - 1;
    int16_t ww = std::max(xp2 + w2, xp1 + w1) + 3 - xp;
    int16_t hh = std::max(yp2 + h2, yp1 + h1) + 3 - yp;
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            int xx = -i1 * 18 + i2 * 18 + 1151;
            int yy = i1 * 9 + i2 * 9 + 9;
            if (SData[CurScene][0][i1][i2] > 0) InitialSPic(SData[CurScene][0][i1][i2] / 2, xx, yy, xp, yp, ww, hh, 0);
            else if (SData[CurScene][0][i1][i2] < 0) InitNewPic(-SData[CurScene][0][i1][i2] / 2, xx, yy, xp, yp, ww, hh, 0);
            if (SData[CurScene][1][i1][i2] > 0) InitialSPic(SData[CurScene][1][i1][i2] / 2, xx, yy - SData[CurScene][4][i1][i2], xp, yp, ww, hh, 0);
            else if (SData[CurScene][1][i1][i2] < 0) InitNewPic(-SData[CurScene][1][i1][i2] / 2, xx, yy - SData[CurScene][4][i1][i2], xp, yp, ww, hh, 0);
            if (SData[CurScene][2][i1][i2] > 0) InitialSPic(SData[CurScene][2][i1][i2] / 2, xx, yy - SData[CurScene][5][i1][i2], xp, yp, ww, hh, 0);
            else if (SData[CurScene][2][i1][i2] < 0) InitNewPic(-SData[CurScene][2][i1][i2] / 2, xx, yy - SData[CurScene][5][i1][i2], xp, yp, ww, hh, 0);
            if (SData[CurScene][3][i1][i2] >= 0)
            {
                int ev = SData[CurScene][3][i1][i2];
                if (DData[CurScene][ev][5] > 0) InitialSPic(DData[CurScene][ev][5] / 2, xx, yy - SData[CurScene][4][i1][i2], xp, yp, ww, hh, 0);
                if (DData[CurScene][ev][5] < 0) InitNewPic(-DData[CurScene][ev][5] / 2, xx, yy - SData[CurScene][4][i1][i2], xp, yp, ww, hh, 0);
            }
        }
}

void LoadScenePart(int x, int y)
{
    if (rs == 0) randomcount = rand() % 640;
    for (int i1 = 0; i1 < screen->w; i1++)
        for (int i2 = 0; i2 < screen->h; i2++)
        {
            int sx = x + i1, sy = y + i2;
            if (sx < 0 || sy < 0 || sx >= 2304 || sy >= 1152) { putpixel(screen, i1, i2, 0); continue; }
            uint32 pix = SceneImg[sx][sy];
            if (Water >= 0)
            {
                int b = (i2 + Water / 3) % 60;
                b = snowalpha[0][b];
                if (b > 128) b -= 256;
                int sx2 = x + i1 - b;
                if (sx2 >= 0 && sx2 < 2304) pix = SceneImg[sx2][sy];
            }
            else if (snow >= 0)
            {
                int b = i1 + randomcount;
                if (b >= 640) b -= 640;
                b = snowalpha[i2][b];
                if (b == 1) pix = ColColor(0xFF);
            }
            else if (fog)
            {
                int b = i1 + randomcount;
                if (b >= 640) b -= 640;
                uint32 alphe = snowalpha[i2][b];
                uint8_t p1 = pix & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = (pix >> 16) & 0xFF, p4 = (pix >> 24) & 0xFF;
                p1 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p1) / 100);
                p2 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p2) / 100);
                p3 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p3) / 100);
                p4 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p4) / 100);
                pix = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
            }
            else if (rain >= 0)
            {
                int b = i1 + randomcount;
                if (b >= 640) b -= 640;
                b = snowalpha[i2][b];
                if (b == 1)
                {
                    uint32 alphe = 50;
                    uint8_t p1 = pix & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = (pix >> 16) & 0xFF, p4 = (pix >> 24) & 0xFF;
                    p1 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p1) / 100);
                    p2 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p2) / 100);
                    p3 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p3) / 100);
                    p4 = static_cast<uint8_t>((alphe * 0xFF + (100 - alphe) * p4) / 100);
                    pix = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
                }
            }
            else if (showblackscreen)
            {
                uint32 alphe = snowalpha[i2][i1];
                if (alphe >= 100) pix = 0;
                else if (alphe > 0)
                {
                    uint8_t p1 = pix & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = (pix >> 16) & 0xFF, p4 = (pix >> 24) & 0xFF;
                    p1 = static_cast<uint8_t>(((100 - alphe) * p1) / 100);
                    p2 = static_cast<uint8_t>(((100 - alphe) * p2) / 100);
                    p3 = static_cast<uint8_t>(((100 - alphe) * p3) / 100);
                    p4 = static_cast<uint8_t>(((100 - alphe) * p4) / 100);
                    pix = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
                }
            }
            putpixel(screen, i1, i2, pix);
        }
}

// ==== 战场绘制 ====

void DrawWholeBField()
{
    DrawBfieldWithoutRole(Bx, By);
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            if (BField[2][i1][i2] >= 0 && BRole[BField[2][i1][i2]].rnum >= 0 && BRole[BField[2][i1][i2]].Show == 0)
                DrawRoleOnBfield(i1, i2);
            else if (BField[5][i1][i2] >= 0 && BRole[BField[5][i1][i2]].rnum >= 0 && BRole[BField[5][i1][i2]].Show == 0)
                DrawRoleOnBfield(i1, i2);
        }
}

void DrawBfieldWithoutRole(int x, int y)
{
    LoadBfieldPart(-x * 18 + y * 18 + 1151 - CENTER_X, x * 9 + y * 9 + 9 - CENTER_Y);
}

void DrawRoleOnBfield(int x, int y)
{
    int num = 0;
    if (BField[2][x][y] >= 0)
        num = RRole[BRole[BField[2][x][y]].rnum].HeadNum * 4 + BRole[BField[2][x][y]].Face + BEGIN_BATTLE_ROLE_PIC;
    else if (BField[5][x][y] >= 0)
        num = RRole[BRole[BField[5][x][y]].rnum].HeadNum * 4 + BRole[BField[5][x][y]].Face + BEGIN_BATTLE_ROLE_PIC;
    int offset = (num == 0) ? 0 : WIdx[num - 1];
    uint8_t* pp = &WPic[offset];
    int16_t w = *reinterpret_cast<int16_t*>(pp); pp += 2;
    int16_t h = *reinterpret_cast<int16_t*>(pp); pp += 2;
    int16_t xs = *reinterpret_cast<int16_t*>(pp); pp += 2;
    int16_t ys_ = *reinterpret_cast<int16_t*>(pp);
    TPosition pos = GetPositionOnScreen(x, y, Bx, By);
    if (BField[2][x][y] >= 0 && BRole[BField[2][x][y]].Show == 0 && BRole[BField[2][x][y]].rnum >= 0)
        DrawBRolePic(num, pos.x - xs, pos.y - ys_, w, h, pos.x, pos.y, 0, 1);
    else if (BField[5][x][y] >= 0 && BRole[BField[5][x][y]].Show == 0 && BRole[BField[5][x][y]].rnum >= 0)
        DrawBRolePic(num, pos.x - xs, pos.y - ys_, w, h, pos.x, pos.y, 0, 1);
    for (int i1 = x; i1 < 64; i1++)
        for (int i2 = y; i2 < 64; i2++)
        {
            TPosition pos1 = GetPositionOnScreen(i1, i2, Bx, By);
            if (BField[1][i1][i2] > 0)
                DrawBPic(BField[1][i1][i2] / 2, pos.x - xs, pos.y - ys_, w, h, pos1.x, pos1.y, 0, 2);
        }
}

void InitialWholeBField()
{
    for (int i1 = 0; i1 < 2304; i1++)
        for (int i2 = 0; i2 < 1152; i2++)
            BFieldImg[i1][i2] = 0;
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            int x = -i1 * 18 + i2 * 18 + 1151;
            int y = i1 * 9 + i2 * 9 + 9;
            InitialBPic(BField[0][i1][i2] / 2, x, y);
            if (BField[1][i1][i2] > 0)
                InitialBPic(BField[1][i1][i2] / 2, x, y);
        }
}

void LoadBfieldPart(int x, int y)
{
    for (int i1 = 0; i1 < screen->w; i1++)
        for (int i2 = 0; i2 < screen->h; i2++)
            if (x + i1 >= 0 && y + i2 >= 0 && x + i1 < 2304 && y + i2 < 1152)
                putpixel(screen, i1, i2, BFieldImg[x + i1][y + i2]);
            else
                putpixel(screen, i1, i2, 0);
}

void DrawBFieldWithCursor(int AttAreaType, int step, int range)
{
    Redraw();
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            BField[4][i1][i2] = 0;
            TPosition pos = GetPositionOnScreen(i1, i2, Bx, By);
            if (BField[0][i1][i2] <= 0) continue;
            bool inRange = false, inStep = false;
            switch (AttAreaType)
            {
            case 0: inRange = (abs(i1 - Ax) + abs(i2 - Ay) <= range);
                    inStep = (abs(i1 - Bx) + abs(i2 - By) <= step) && (BField[3][i1][i2] >= 0); break;
            case 1: inRange = ((i1 == Bx && abs(i2 - By) <= step && (i2 - By) * (Ay - By) > 0) ||
                               (i2 == By && abs(i1 - Bx) <= step && (i1 - Bx) * (Ax - Bx) > 0));
                    inStep = ((i1 == Bx && abs(i2 - By) <= step) || (i2 == By && abs(i1 - Bx) <= step)); break;
            case 2: inRange = ((i1 == Bx && abs(i2 - By) <= step) || (i2 == By && abs(i1 - Bx) <= step) ||
                               (abs(i1 - Bx) == abs(i2 - By) && abs(i1 - Bx) <= range)); inStep = false; break;
            case 3: inRange = (abs(i1 - Ax) <= range && abs(i2 - Ay) <= range);
                    inStep = (abs(i1 - Bx) + abs(i2 - By) <= step) && (BField[0][i1][i2] >= 0); break;
            case 4: { bool cond = (abs(i1 - Bx) + abs(i2 - By) <= step) && (abs(i1 - Bx) != abs(i2 - By));
                    bool dir = ((i1 - Bx) * (Ax - Bx) > 0 && abs(i1 - Bx) > abs(i2 - By)) ||
                               ((i2 - By) * (Ay - By) > 0 && abs(i1 - Bx) < abs(i2 - By));
                    inRange = cond && dir; inStep = cond && !dir; break; }
            case 5: { bool cond = (abs(i1 - Bx) <= step && abs(i2 - By) <= step && abs(i1 - Bx) != abs(i2 - By));
                    bool dir = ((i1 - Bx) * (Ax - Bx) > 0 && abs(i1 - Bx) > abs(i2 - By)) ||
                               ((i2 - By) * (Ay - By) > 0 && abs(i1 - Bx) < abs(i2 - By));
                    inRange = cond && dir; inStep = cond && !dir; break; }
            case 6: inRange = (abs(i1 - Ax) + abs(i2 - Ay) <= range);
                    inStep = (abs(i1 - Bx) + abs(i2 - By) <= step) && (abs(i1 - Bx) + abs(i2 - By) > 3) && (BField[3][i1][i2] >= 0); break;
            case 7: {
                    if (i1 == Bx && i2 == By) { inRange = true; break; }
                    bool inS = (abs(i1 - Bx) + abs(i2 - By) <= step) && (BField[3][i1][i2] >= 0);
                    if (inS && abs(i1 - Bx) <= abs(Ax - Bx) && abs(i2 - By) <= abs(Ay - By))
                    {
                        if (Ax != Bx && abs(Ax - Bx) > abs(Ay - By) && (i1 - Bx) * (Ax - Bx) > 0 &&
                            i2 == static_cast<int>(std::round(static_cast<double>(i1 - Bx) * (Ay - By) / (Ax - Bx)) + By))
                            inRange = true;
                        else if (Ay != By && abs(Ax - Bx) <= abs(Ay - By) && (i2 - By) * (Ay - By) > 0 &&
                            i1 == static_cast<int>(std::round(static_cast<double>(i2 - By) * (Ax - Bx) / (Ay - By)) + Bx))
                            inRange = true;
                        else inStep = inS;
                    }
                    else inStep = false;
                    if (!inRange && !inStep) { DrawBPic(BField[0][i1][i2] / 2, pos.x, pos.y, -1); continue; }
                    break; }
            }
            if (inRange) { DrawBPic(BField[0][i1][i2] / 2, pos.x, pos.y, 1); BField[4][i1][i2] = 1; }
            else if (inStep) DrawBPic(BField[0][i1][i2] / 2, pos.x, pos.y, 0);
            else DrawBPic(BField[0][i1][i2] / 2, pos.x, pos.y, -1);
        }
    // Second pass: draw buildings and roles on top
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            TPosition pos = GetPositionOnScreen(i1, i2, Bx, By);
            if (BField[1][i1][i2] > 0) DrawBPic(BField[1][i1][i2] / 2, pos.x, pos.y, 0, 0);
            int bnum = BField[2][i1][i2];
            if (bnum >= 0 && BRole[bnum].Dead == 0 && BRole[bnum].rnum >= 0)
            {
                if (BField[4][i1][i2] > 0 && BRole[bnum].Team != BRole[BField[2][Bx][By]].Team)
                    HighLight = true;
                DrawBRolePic(RRole[BRole[bnum].rnum].HeadNum * 4 + BRole[bnum].Face + BEGIN_BATTLE_ROLE_PIC, pos.x, pos.y, 0, 0);
                HighLight = false;
            }
        }
    showprogress();
}

void DrawBFieldWithEft(FILE* f, int Epicnum, int bigami, int level)
{
    TPic image = GetPngPic(f, Epicnum);
    DrawBfieldWithoutRole(Bx, By);
    int n = 0;
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
            if (BField[2][i1][i2] >= 0 && BRole[BField[2][i1][i2]].Show == 0 && BRole[BField[2][i1][i2]].rnum >= 0)
                DrawRoleOnBfield(i1, i2);
    if (bigami == 0)
    {
        for (int i1 = 0; i1 < 64; i1++)
            for (int i2 = 0; i2 < 64; i2++)
            {
                TPosition pos = GetPositionOnScreen(i1, i2, Bx, By);
                if (BField[4][i1][i2] > 0)
                {
                    n++;
                    DrawEftPic(image, pos.x, pos.y, 0);
                }
            }
        n = 300 - n * 3;
        if (image.pic && (image.pic->w > 120 || image.pic->h > 120)) n -= 5;
        n /= 10;
        if (n > 0) SDL_Delay((n * GameSpeed) / 10);
    }
    else
    {
        TPosition pos = GetPositionOnScreen(Ax, Ay, Bx, By);
        if (BField[4][Ax][Ay] > 0) DrawEftPic(image, pos.x, pos.y, level);
        n = 30 + (image.black - 1) * 10;
        SDL_Delay(((n + 5) * GameSpeed) / 10);
    }
    SDL_DestroySurface(image.pic);
}

void DrawBFieldWithAction(FILE* f, int bnum, int Apicnum)
{
    DrawBfieldWithoutRole(Bx, By);
    TPic image = GetPngPic(f, Apicnum);
    for (int i1 = 0; i1 < 64; i1++)
        for (int i2 = 0; i2 < 64; i2++)
        {
            if (BField[2][i1][i2] >= 0 && BRole[BField[2][i1][i2]].Show == 0 && BField[2][i1][i2] != bnum && BRole[BField[2][i1][i2]].rnum >= 0)
                DrawRoleOnBfield(i1, i2);
            if (BField[2][i1][i2] == bnum)
            {
                TPosition pos1 = GetPositionOnScreen(i1, i2, Bx, By);
                for (int ii1 = i1; ii1 < 64; ii1++)
                    for (int ii2 = i2; ii2 < 64; ii2++)
                    {
                        TPosition pos = GetPositionOnScreen(ii1, ii2, Bx, By);
                        if (ii1 == i1 && ii2 == i2)
                            drawPngPic(image, pos1.x, pos1.y, 1);
                        if (BField[1][ii1][ii2] > 0)
                            DrawBPic(BField[1][ii1][ii2] / 2, pos1.x - image.x, pos1.y - image.y, image.pic->w, image.pic->h, pos.x, pos.y, 0);
                    }
            }
        }
    SDL_DestroySurface(image.pic);
}

// ==== 菜单系统 ====

static void ShowMedcineMenu(int rnum, int menu)
{
    int x = 115, y = 94;
    int max = 0;
    for (int i = 0; i < 6; i++) if (TeamList[i] >= 0) max++;
    display_imgFromSurface(MAGIC_PIC.pic, x + 248, y - 60, x + 248, y - 60, 276, 200);
    DrawRectangle(x + 250, y - 60, 200, 10 + max * 22, 0, ColColor(0, 255), 30);
    for (int i = 0; i < max; i++)
    {
        std::wstring name = GBKToUnicode(RRole[TeamList[i]].Name);
        std::wstring hp = std::to_wstring(RRole[TeamList[i]].CurrentHP) + L"/" + std::to_wstring(RRole[TeamList[i]].MaxHP);
        if (i == menu)
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), x + 250, y - 55 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
            DrawEngShadowText(reinterpret_cast<uint16_t*>(hp.data()), x + 320, y - 55 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
        }
        else
        {
            DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), x + 250, y - 55 + 22 * i, ColColor(0, 5), ColColor(0, 7));
            DrawEngShadowText(reinterpret_cast<uint16_t*>(hp.data()), x + 320, y - 55 + 22 * i, ColColor(0, 5), ColColor(0, 7));
        }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

void NewMenuSystem()
{
    int menu = 0;
    NewShowMenuSystem(menu);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { menu = (menu + 1) % 4; NewShowMenuSystem(menu); }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { menu = (menu + 3) % 4; NewShowMenuSystem(menu); }
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); break; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                switch (menu)
                {
                case 0: if (NewMenuLoad()) break; else NewShowMenuSystem(menu); continue;
                case 1: if (NewMenuSave()) break; else NewShowMenuSystem(menu); continue;
                case 2: NewMenuVolume(); NewShowMenuSystem(menu); continue;
                case 3: NewMenuQuit(); NewShowMenuSystem(menu); continue;
                }
                break;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) { Redraw(); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); break; }
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (event.button.button == SDL_BUTTON_LEFT && xm >= 112 && xm < 780 && ym > 25 && ym < 415)
            {
                switch (menu)
                {
                case 0: if (NewMenuLoad()) break; else NewShowMenuSystem(menu); continue;
                case 1: if (NewMenuSave()) break; else NewShowMenuSystem(menu); continue;
                case 2: NewMenuVolume(); NewShowMenuSystem(menu); continue;
                case 3: NewMenuQuit(); NewShowMenuSystem(menu); continue;
                }
                break;
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 112 && xm < 780 && ym > 25 && ym < 415)
            {
                int m = (ym - 25) / 101;
                if (m > 3) m = 3; if (m < 0) m = 0;
                if (m != menu) { menu = m; NewShowMenuSystem(menu); }
            }
        }
    }
}

void SelectShowStatus()
{
    int max = 0, menu = 0;
    MenuString.clear();
    for (int i = 0; i < 6; i++)
        if (TeamList[i] >= 0) { MenuString.push_back(GBKToUnicode(RRole[TeamList[i]].Name)); max++; }
    NewShowStatus(TeamList[menu]);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            if (menu >= max) menu = 0; if (menu < 0) menu = max - 1;
            NewShowStatus(TeamList[menu]);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm > 15 && ym > 15 && xm < 105 && ym < 25 + max * 22)
            {
                int m = (ym - 25) / 22;
                if (m != menu) { menu = m; NewShowStatus(TeamList[menu]); }
            }
        }
    }
}

void NewShowStatus(int rnum)
{
    int max = static_cast<int>(MenuString.size());
    int x = 90, y = 0;
    display_imgFromSurface(STATE_PIC.pic, 0, 0);
    if (!isbattle)
    {
        DrawRectangle(15, 15, 90, 10 + max * 22, 0, ColColor(0, 255), 30);
        for (int i = 0; i < max; i++)
        {
            auto* p = reinterpret_cast<uint16_t*>(MenuString[i].data());
            if (TeamList[i] == rnum)
                DrawShadowText(p, 0, 20 + 22 * i, ColColor(0, 0x64), ColColor(0, 0x66));
            else
                DrawShadowText(p, 0, 20 + 22 * i, ColColor(0, 5), ColColor(0, 7));
        }
    }
    DrawHeadPic(RRole[rnum].HeadNum, 137, 88);
    std::wstring str = GBKToUnicode(RRole[rnum].Name);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 115, 93, ColColor(0x64), ColColor(0x66));

    std::wstring strs[] = { L" 生命", L" 內力", L" 體力", L" 等級", L" 經驗", L" 升級",
        L" 攻擊", L" 防禦", L" 輕功", L" 醫療能力", L" 用毒能力", L" 解毒能力",
        L" 拳掌功夫", L" 御劍能力", L" 耍刀技巧", L" 奇門兵器", L" 暗器技巧" };
    for (int i = 3; i <= 5; i++)
        DrawShadowText(reinterpret_cast<uint16_t*>(strs[i].data()), x + 25, y + 94 + 21 * (i - 2), ColColor(0, 0x21), ColColor(0, 0x23));
    for (int i = 6; i <= 16; i++)
        DrawShadowText(reinterpret_cast<uint16_t*>(strs[i].data()), x + 25, y + 115 + 21 * (i - 3), ColColor(0, 0x63), ColColor(0, 0x66));

    str = std::format(L"{:4d}", RRole[rnum].Level);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 145, y + 115, ColColor(5), ColColor(7));
    UpdateHpMp(rnum, x + 80 + 25, y - 85 + 94);
    str = std::format(L"{:5d}", static_cast<uint16_t>(RRole[rnum].Exp));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 135, y + 136, ColColor(5), ColColor(7));
    if (RRole[rnum].Level == MAX_LEVEL) str = L"    =";
    else str = std::format(L"{:5d}", static_cast<uint16_t>(LevelUpList[RRole[rnum].Level - 1]));
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 135, y + 157, ColColor(5), ColColor(7));

    int addatk = 0, adddef = 0, addspd = 0;
    for (int i = 0; i < 5; i++)
        if (RRole[rnum].Equip[i] >= 0) { addatk += RItem[RRole[rnum].Equip[i]].AddAttack; adddef += RItem[RRole[rnum].Equip[i]].AddDefence; addspd += RItem[RRole[rnum].Equip[i]].AddSpeed; }
    if (CheckEquipSet(RRole[rnum].Equip[0], RRole[rnum].Equip[1], RRole[rnum].Equip[2], RRole[rnum].Equip[3]) == 5)
    { addatk += 50; addspd += 30; adddef -= 25; }
    auto fmtStat = [](int base, int add) -> std::wstring {
        if (add > 0) return std::format(L"{:4d}+{}", base, add);
        if (add < 0) return std::format(L"{:4d}-{}", base, -add);
        return std::format(L"{:4d}", base);
    };
    str = fmtStat(GetRoleAttack(rnum, false), addatk);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 145, y + 115 + 21 * 3, ColColor(5), ColColor(7));
    str = fmtStat(GetRoleDefence(rnum, false), adddef);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 145, y + 115 + 21 * 4, ColColor(5), ColColor(7));
    str = fmtStat(GetRoleSpeed(rnum, false), addspd);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 145, y + 115 + 21 * 5, ColColor(5), ColColor(7));
    auto showStat = [&](int val, int row) { str = std::format(L"{:4d}", val); DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 145, y + 115 + 21 * row, ColColor(5), ColColor(7)); };
    showStat(GetRoleMedcine(rnum, true), 6);
    showStat(GetRoleUsePoi(rnum, true), 7);
    showStat(GetRoleMedPoi(rnum, true), 8);
    showStat(GetRoleFist(rnum, true), 9);
    showStat(GetRoleSword(rnum, true), 10);
    showStat(GetRoleKnife(rnum, true), 11);
    showStat(GetRoleUnusual(rnum, true), 12);
    showStat(GetRoleHidWeapon(rnum, true), 13);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    if (isbattle) WaitAnyKey();
}

void SelectShowMagic()
{
    int max = 0, menu = 0;
    MenuString.clear();
    for (int i = 0; i < 6; i++)
        if (TeamList[i] >= 0) { MenuString.push_back(GBKToUnicode(RRole[TeamList[i]].Name)); max++; }
    NewShowMagic(TeamList[menu]);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            if (menu >= max) menu = 0; if (menu < 0) menu = max - 1;
            NewShowMagic(TeamList[menu]);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
                if (InModeMagic(TeamList[menu])) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 350) { if (InModeMagic(TeamList[menu])) break; }
            else if (xm > 15 && ym > 15 && xm < 105 && ym < 25 + max * 22)
            {
                int m = (ym - 25) / 22;
                if (m != menu) { menu = m; NewShowMagic(TeamList[menu]); }
            }
        }
    }
}

void NewShowMagic(int rnum)
{
    int max = static_cast<int>(MenuString.size());
    int x = 90, y = 0;
    display_imgFromSurface(MAGIC_PIC.pic, 0, 0);
    if (Where != 2)
    {
        DrawRectangle(15, 15, 90, 10 + max * 22, 0, ColColor(255), 30);
        for (int i = 0; i < max; i++)
        {
            auto* p = reinterpret_cast<uint16_t*>(MenuString[i].data());
            if (TeamList[i] == rnum) DrawShadowText(p, 0, 20 + 22 * i, ColColor(0x64), ColColor(0x66));
            else DrawShadowText(p, 0, 20 + 22 * i, ColColor(5), ColColor(7));
        }
    }
    DrawHeadPic(RRole[rnum].HeadNum, 137, 88);
    std::wstring str = GBKToUnicode(RRole[rnum].Name);
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), 115, 93, ColColor(0x64), ColColor(0x66));
    if (RRole[rnum].PracticeBook >= 0)
    {
        str = GBKToUnicode(RItem[RRole[rnum].PracticeBook].Name);
        DrawItemPic(RRole[rnum].PracticeBook, 136, 208);
    }
    else str = L" 無";
    std::wstring label = L" 修煉物品";
    DrawShadowText(reinterpret_cast<uint16_t*>(label.data()), x + 110, y + 216, ColColor(0x21), ColColor(0x23));
    DrawShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 110, y + 237, ColColor(0x64), ColColor(0x66));
    UpdateHpMp(rnum, x + 25, y + 94);
    ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true);
}

void ShowMagic(int rnum, int num, int x1, int y1, int w, int h, bool showit)
{
    int x = 90, y = 0;
    display_imgFromSurface(MAGIC_PIC.pic, x + 247, y + 32, x + 247, y + 32, 276, 408 - y);
    display_imgFromSurface(MAGIC_PIC.pic, x + 30, y + 300, x + 30, y + 300, 300, 140);
    std::wstring skillstr[] = { L" 醫療", L" 解毒", L" 用毒", L" 抗毒", L" 毒攻", L" " };
    std::wstring knowmagic = L" ————所會武功————";
    std::wstring skill = L" ————特殊技能————";
    DrawShadowText(reinterpret_cast<uint16_t*>(skill.data()), x + 247, y + 36, ColColor(0, 0x21), ColColor(0, 0x23));
    DrawShadowText(reinterpret_cast<uint16_t*>(knowmagic.data()), x + 247, y + 102, ColColor(0, 0x21), ColColor(0, 0x23));
    int lv[6] = {};
    lv[0] = GetRoleMedcine(rnum, true); lv[1] = GetRoleMedPoi(rnum, true); lv[2] = GetRoleUsePoi(rnum, true);
    lv[3] = GetRoleDefPoi(rnum, true); lv[4] = GetRoleAttPoi(rnum, true);
    for (int i = 0; i < 6; i++)
    {
        bool active = (i < 3 ? lv[i] >= 20 : (i < 5 ? lv[i] > 0 : RRole[rnum].AttTwice > 0));
        uint32 c1 = active ? ColColor(0, 5) : ColColor(0, 0x66);
        uint32 c2 = active ? ColColor(0, 7) : ColColor(0, 0x68);
        DrawShadowText(reinterpret_cast<uint16_t*>(skillstr[i].data()), x + 248 + 78 * (i % 3), y + (i / 3) * 22 + 58, c1, c2);
    }
    std::wstring magicstr[10];
    for (int i = 0; i < 10; i++)
    {
        if (RRole[rnum].Magic[i] > 0) magicstr[i] = GBKToUnicode(RMagic[RRole[rnum].Magic[i]].Name);
        else magicstr[i] = L" ";
        uint32 c1 = (RRole[rnum].Magic[i] == RRole[rnum].Gongti) ? ColColor(0, 21) : ColColor(0, 5);
        uint32 c2 = (RRole[rnum].Magic[i] == RRole[rnum].Gongti) ? ColColor(0, 24) : ColColor(0, 7);
        DrawShadowText(reinterpret_cast<uint16_t*>(magicstr[i].data()), x + 248 + 118 * (i % 2), y + (i / 2) * 22 + 124, c1, c2);
    }
    if (num >= 6 && RRole[rnum].Magic[num - 6] > 0)
    {
        DrawShadowText(reinterpret_cast<uint16_t*>(magicstr[num - 6].data()), x + 248 + 118 * ((num - 6) % 2), y + ((num - 6) / 2) * 22 + 124, ColColor(0, 0x63), ColColor(0, 0x66));
        DrawShadowText(reinterpret_cast<uint16_t*>(magicstr[num - 6].data()), x + 35, y + 300, ColColor(0, 5), ColColor(0, 7));
    }
    if (showit) SDL_UpdateRect2(screen, x1, y1, w, h);
}

bool InModeMagic(int rnum)
{
    int max = 0, num = 0;
    for (int i = 0; i < 10; i++) if (RRole[rnum].Magic[i] > 0) max++;
    ShowMagic(rnum, 0, 0, 0, screen->w, screen->h, true);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { if (num <= 5 && num >= 0) num -= 3; else if (num >= 6) num -= 2; }
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { if (num >= 3 && num <= 5) num = (max == 0) ? num - 3 : 6; else if (num <= 2 && num >= 0) num += 3; else if (num >= 6) num += 2; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) num++;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) num--;
            if (num < 0) num = (max == 0) ? num + 3 : max + 5;
            if (num > max + 5) num = 0;
            ShowMagic(rnum, num, 0, 0, screen->w, screen->h, true);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); return false; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                if (!isbattle)
                {
                    if (num == 0 && GetRoleMedcine(rnum, true) >= 20) MenuMedcine(rnum);
                    if (num == 1 && GetRoleMedPoi(rnum, true) >= 20) MenuMedPoision(rnum);
                    if (num > 5 && RRole[rnum].Magic[num - 6] >= 0 && RMagic[RRole[rnum].Magic[num - 6]].MagicType == 5)
                    { SetGongti(rnum, RRole[rnum].Magic[num - 6]); NewShowMagic(rnum); }
                }
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); return true; }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm <= 116) { return false; }
            int np = num;
            if (xm > 337 && ym > 57 && xm < 573 && ym < 101) num = ((ym - 57) / 22) * 3 + (xm - 337) / 78;
            else if (xm > 337 && ym > 124 && xm < 573 && ym < 234) num = ((ym - 124) / 22) * 2 + (xm - 337) / 118 + 6;
            if (np != num) ShowMagic(rnum, num, 0, 0, screen->w, screen->h, true);
        }
    }
    return false;
}

void UpdateHpMp(int rnum, int x, int y)
{
    std::wstring strs[] = { L" 生命", L" 內力", L" 體力" };
    for (int i = 0; i < 3; i++)
        DrawShadowText(reinterpret_cast<uint16_t*>(strs[i].data()), x, y + 21 * (i + 1), ColColor(0x21), ColColor(0x23));
    uint32 c1, c2;
    if (RRole[rnum].Hurt >= 67) { c1 = ColColor(0x14); c2 = ColColor(0x16); }
    else if (RRole[rnum].Hurt >= 34) { c1 = ColColor(0x10); c2 = ColColor(0x0E); }
    else { c1 = ColColor(5); c2 = ColColor(7); }
    std::wstring str = std::format(L"{:4d}", RRole[rnum].CurrentHP);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 125, y + 21, c1, c2);
    str = L"/";
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 165, y + 21, ColColor(0x63), ColColor(0x66));
    if (RRole[rnum].Poision >= 67) { c1 = ColColor(0x35); c2 = ColColor(0x37); }
    else if (RRole[rnum].Poision >= 34) { c1 = ColColor(0x30); c2 = ColColor(0x32); }
    else { c1 = ColColor(0x21); c2 = ColColor(0x23); }
    str = std::format(L"{:4d}", RRole[rnum].MaxHP);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 175, y + 21, c1, c2);
    DrawRectangleWithoutFrame(x + 65, y + 24, 52, 15, ColColor(0), 30);
    if (RRole[rnum].MaxHP > 0)
        DrawRectangleWithoutFrame(x + 66, y + 25, (50 * RRole[rnum].CurrentHP) / RRole[rnum].MaxHP, 13, c2, 50);
    if (RRole[rnum].MPType == 1) { c1 = ColColor(0x4E); c2 = ColColor(0x50); }
    else if (RRole[rnum].MPType == 0) { c1 = ColColor(5); c2 = ColColor(7); }
    else { c1 = ColColor(0x63); c2 = ColColor(0x66); }
    if (RRole[rnum].MaxMP > 0)
    {
        str = std::format(L"{:4d}/{:4d}", RRole[rnum].CurrentMP, RRole[rnum].MaxMP);
        DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 125, y + 42, c1, c2);
        DrawRectangleWithoutFrame(x + 65, y + 45, 52, 15, ColColor(0), 30);
        DrawRectangleWithoutFrame(x + 66, y + 46, (50 * RRole[rnum].CurrentMP) / RRole[rnum].MaxMP, 13, c2, 50);
    }
    str = std::format(L"{:4d}/{:4d}", RRole[rnum].PhyPower, MAX_PHYSICAL_POWER);
    DrawEngShadowText(reinterpret_cast<uint16_t*>(str.data()), x + 125, y + 63, ColColor(5), ColColor(7));
    DrawRectangleWithoutFrame(x + 65, y + 65, 52, 15, ColColor(0), 30);
    DrawRectangleWithoutFrame(x + 66, y + 66, (50 * RRole[rnum].PhyPower) / MAX_PHYSICAL_POWER, 13, ColColor(0x46), 50);
}

void MenuMedcine(int rnum)
{
    int menu = 0, max = 0;
    for (int i = 0; i < 6; i++) if (TeamList[i] >= 0) max++;
    ShowMedcineMenu(rnum, menu);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            if (menu < 0) menu = max - 1; if (menu >= max) menu = 0;
            ShowMedcineMenu(rnum, menu);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); break; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                if (menu >= 0) { EffectMedcine(rnum, TeamList[menu]); UpdateHpMp(rnum, 115, 94); ShowMedcineMenu(rnum, menu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); break; }
            if (event.button.button == SDL_BUTTON_LEFT && menu >= 0)
            { EffectMedcine(rnum, TeamList[menu]); UpdateHpMp(rnum, 115, 94); ShowMedcineMenu(rnum, menu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
        }
    }
}

void MenuMedPoision(int rnum)
{
    int menu = 0, max = 0;
    for (int i = 0; i < 6; i++) if (TeamList[i] >= 0) max++;
    ShowMedcineMenu(rnum, menu); // reuse same display
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            if (menu < 0) menu = max - 1; if (menu >= max) menu = 0;
            ShowMedcineMenu(rnum, menu);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); break; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                if (menu >= 0) { EffectMedPoision(rnum, TeamList[menu]); UpdateHpMp(rnum, 115, 94); ShowMedcineMenu(rnum, menu); SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h); }
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) { ShowMagic(rnum, -1, 0, 0, screen->w, screen->h, true); break; }
        }
    }
}

TPic GetPngPic(const std::string& filename, int num)
{
    TPic result = {};
    std::string path = AppPath + filename;
    FILE* fp = nullptr;
    fopen_s(&fp, path.c_str(), "rb");
    if (!fp) return result;
    result = GetPngPic(fp, num);
    fclose(fp);
    return result;
}

void drawPngPic(TPic image, int x, int y, int w, int h, int px, int py, int mask)
{
    if (image.pic == nullptr) return;
    int bpp = 4;
    int xs = px - image.x;
    int ys = py - image.y;
    int iw = (w > 0) ? w : image.pic->w;
    int ih = (h > 0) ? h : image.pic->h;
    int ox = (w > 0) ? x : 0;
    int oy = (h > 0) ? y : 0;
    for (int iy = 0; iy < ih; iy++)
        for (int ix = 0; ix < iw; ix++)
        {
            int sx = ix + ox, sy = iy + oy;
            if (sx >= image.pic->w || sy >= image.pic->h) continue;
            auto* pp = reinterpret_cast<uint32_t*>(
                static_cast<uint8_t*>(image.pic->pixels) + sy * image.pic->pitch + sx * bpp);
            uint32 c = *pp;
            int alpha = (c >> 24) & 0xFF;
            if (alpha == 0) continue;
            int dx = xs + ix, dy = ys + iy;
            if (dx < 0 || dy < 0 || dx >= screen->w || dy >= screen->h) continue;
            if (mask == 2 && MaskArray[dx][dy] != 1) continue;
            uint8_t col1 = (c >> 16) & 0xFF, col2 = (c >> 8) & 0xFF, col3 = c & 0xFF;
            uint32 pix = getpixel(screen, dx, dy);
            uint8_t p1 = (pix >> 16) & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = pix & 0xFF;
            BlendRGB255(p1, p2, p3, col1, col2, col3, alpha);
            putpixel(screen, dx, dy, (p1 << 16) | (p2 << 8) | p3 | AMask);
            if (mask == 1 && dx < 2304 && dy < 1152) MaskArray[dx][dy] = 1;
        }
}

void drawPngPic(TPic image, int px, int py, int mask) { drawPngPic(image, 0, 0, 0, 0, px, py, mask); }

SDL_Surface* ReadPicFromByte(uint8_t* p_byte, int size)
{
    auto io = SDL_IOFromMem(p_byte, size);
    return IMG_Load_IO(io, true);
}

std::string Simplified2Traditional(const std::string& mSimplified) { return mSimplified; }
std::wstring Traditional2Simplified(const std::wstring& mTraditional) { return mTraditional; }

void NewShowMenuSystem(int menu)
{
    display_imgFromSurface(SYSTEM_PIC.pic, 0, 0);
    std::wstring word[] = { L" ——————————讀取進度——————————", L" ——————————保存進度——————————",
        L" ——————————音樂音量——————————", L" ——————————退出離開——————————" };
    for (int i = 0; i < 4; i++)
    {
        auto* p = reinterpret_cast<uint16_t*>(word[i].data());
        if (i == menu) { DrawText_(screen, p, 113, 25 + 101 * i, ColColor(0x64)); DrawText_(screen, p, 112, 25 + 101 * i, ColColor(0x66)); }
        else { DrawText_(screen, p, 113, 25 + 101 * i, ColColor(5)); DrawText_(screen, p, 112, 25 + 101 * i, ColColor(7)); }
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

bool NewMenuSave()
{
    std::wstring word[] = { L" 進度一", L" 進度二", L" 進度三", L" 進度四", L" 進度五" };
    int menu = 0;
    NewShowSelect(1, menu, word, 5, 97);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu = (menu + 1) % 5;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu = (menu + 4) % 5;
            NewShowSelect(1, menu, word, 5, 97);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { SaveR(menu + 1); return true; }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
            if (event.button.button == SDL_BUTTON_LEFT) { int xm, ym; SDL_GetMouseState2(xm, ym); if (xm >= 112 && xm < 572 && ym > 150 && ym < 180) { SaveR(menu + 1); return true; } }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 112 && xm < 572 && ym > 150 && ym < 180) { int m = std::clamp((xm - 117) / 97, 0, 4); if (m != menu) { menu = m; NewShowSelect(1, menu, word, 5, 97); } }
        }
    }
    return false;
}

void NewShowSelect(int row, int menu, const std::wstring* word, int count, int Width)
{
    display_imgFromSurface(SYSTEM_PIC.pic, 0, 0);
    for (int i = 0; i < count; i++)
    {
        auto* p = const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(word[i].data()));
        if (i == menu) { DrawText_(screen, p, 119 + Width * i, 50 + 101 * row, ColColor(0x64)); DrawText_(screen, p, 118 + Width * i, 50 + 101 * row, ColColor(0x66)); }
        else { DrawText_(screen, p, 119 + Width * i, 50 + 101 * row, ColColor(5)); DrawText_(screen, p, 118 + Width * i, 50 + 101 * row, ColColor(7)); }
    }
    SDL_UpdateRect2(screen, 115, 50 + 101 * row, 525, 25);
}

bool NewMenuLoad()
{
    std::wstring word[] = { L" 進度一", L" 進度二", L" 進度三", L" 進度四", L" 進度五", L" 自動檔" };
    int menu = 0;
    NewShowSelect(0, menu, word, 6, 81);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu = (menu + 1) % 6;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu = (menu + 5) % 6;
            NewShowSelect(0, menu, word, 6, 81);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            { LoadR(menu + 1); if (Where == 1) WalkInScene(0); Redraw(); return true; }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
            if (event.button.button == SDL_BUTTON_LEFT)
            { int xm, ym; SDL_GetMouseState2(xm, ym); if (xm >= 112 && xm < 602 && ym > 49 && ym < 129) { LoadR(menu + 1); if (Where == 1) WalkInScene(0); Redraw(); return true; } }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 112 && xm < 602 && ym > 49 && ym < 129) { int m = std::clamp((xm - 117) / 81, 0, 5); if (m != menu) { menu = m; NewShowSelect(0, menu, word, 6, 81); } }
        }
    }
    return false;
}

void NewMenuVolume()
{
    int w_ = 56;
    std::wstring word[] = { L" 零", L" 一", L" 二", L" 三", L" 四", L" 五", L" 六", L" 七", L" 八" };
    int menu = MusicVolume / 16;
    NewShowSelect(2, menu, word, 9, w_);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu = (menu + 1) % 9;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu = (menu + 8) % 9;
            NewShowSelect(2, menu, word, 9, w_);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            { MusicVolume = menu * 16; if (MusicTrack) MIX_SetTrackGain(MusicTrack, MusicVolume / 100.0f); }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
            if (event.button.button == SDL_BUTTON_LEFT) { MusicVolume = menu * 16; if (MusicTrack) MIX_SetTrackGain(MusicTrack, MusicVolume / 100.0f); }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 112 && xm < 640 && ym > 251 && ym < 331) { int m = std::clamp((xm - 117) / w_, 0, 8); if (m != menu) { menu = m; NewShowSelect(2, menu, word, 9, w_); } }
        }
    }
}

void NewMenuQuit()
{
    std::wstring word[] = { L" 取消", L" 退出" };
    int menu = -1;
    NewShowSelect(3, menu, word, 2, 56);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) menu = (menu + 1) % 2;
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) menu = (menu + 1) % 2;
            NewShowSelect(3, menu, word, 2, 56);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) { if (menu == 1) Quit(); break; }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
            if (event.button.button == SDL_BUTTON_LEFT && menu == 1) Quit();
            break;
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            if (xm >= 112 && xm < 640 && ym > 352 && ym < 432) { int m = std::clamp((xm - 117) / 56, 0, 1); if (m != menu) { menu = m; NewShowSelect(3, menu, word, 2, 56); } }
        }
    }
}

void DrawItemPic(int num, int x, int y)
{
    if (num < 0 || num >= static_cast<int>(ITEM_PIC.size())) return;
    if (ITEM_PIC[num].pic == nullptr)
        ITEM_PIC[num] = GetPngPic(ITEMS_file, num);
    drawPngPic(ITEM_PIC[num], x, y, 0);
}

void ShowMap()
{
    // Simplified version - draw map background and wait for key
    drawPngPic(MAP_PIC, 0, 30, 640, 380, 0, 30, 0);
    SDL_UpdateRect2(screen, 0, 0, 640, 440);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
        }
    }
}

void NewMenuEsc()
{
    int x = 270, y = 50 + 117;
    int N = 102;
    int positionX[6] = { x, x + N, x + N, x, x - N, x - N };
    int positionY[6] = { y - 117, y - 58, y + 58, y + 117, y + 58, y - 58 };
    Redraw();
    int menu = 0;
    showNewMenuEsc(menu, positionX, positionY);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) { if (menu <= 2) menu++; else if (menu >= 4) menu--; }
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) { if (menu >= 1 && menu <= 3) menu--; else if (menu == 4) menu = 5; else if (menu == 5) menu = 0; }
            if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) { if (menu == 0) menu = 1; else if (menu == 3 || menu == 4) menu--; else if (menu == 5) menu = 0; }
            if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) { if (menu == 0) menu = 5; else if (menu == 3 || menu == 2) menu++; else if (menu == 5 || menu == 1) menu--; }
            showNewMenuEsc(menu, positionX, positionY);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) { resetpallet(); break; }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE)
            {
                resetpallet(0);
                switch (menu)
                {
                case 0: SelectShowMagic(); break;
                case 1: SelectShowStatus(); break;
                case 2: NewMenuSystem(); resetpallet(); event.key.key = 0; return;
                case 3: NewMenuTeammate(); break;
                case 4: FourPets(); break;
                case 5: NewMenuItem(); break;
                }
                resetpallet();
                showNewMenuEsc(menu, positionX, positionY);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) { resetpallet(); break; }
            if (event.button.button == SDL_BUTTON_LEFT)
            {
                int xm, ym; SDL_GetMouseState2(xm, ym);
                int m = -1;
                for (int i = 0; i < 6; i++)
                    if (positionX[i] + 10 < xm && positionX[i] + 90 > xm && positionY[i] + 10 < ym && positionY[i] + 90 > ym) { m = i; break; }
                if (m >= 0)
                {
                    resetpallet(0);
                    switch (m) { case 0: SelectShowMagic(); break; case 1: SelectShowStatus(); break; case 2: NewMenuSystem(); resetpallet(); event.key.key = 0; return; case 3: NewMenuTeammate(); break; case 4: FourPets(); break; case 5: NewMenuItem(); break; }
                    resetpallet();
                    showNewMenuEsc(menu, positionX, positionY);
                }
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_MOTION)
        {
            int xm, ym; SDL_GetMouseState2(xm, ym);
            for (int i = 0; i < 6; i++)
                if (positionX[i] + 10 < xm && positionX[i] + 90 > xm && positionY[i] + 10 < ym && positionY[i] + 90 > ym)
                { if (i != menu) { menu = i; showNewMenuEsc(menu, positionX, positionY); } break; }
        }
    }
    Redraw();
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
    event.key.key = 0;
}

void showNewMenuEsc(int menu, int* positionX, int* positionY)
{
    Redraw();
    drawPngPic(MenuescBack_PIC, 0, 0, 300, 300, 170, 70, 0);
    for (int i = 0; i < 6; i++)
    {
        if (i == menu)
            drawPngPic(MENUESC_PIC, (i % 3) * 100, (i / 3) * 100, 100, 100, positionX[i], positionY[i], 0);
        else
            drawPngPic(MENUESC_PIC, (i % 3) * 100, (i / 3) * 100 + 200, 100, 100, positionX[i], positionY[i], 0);
    }
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

void DrawEftPic(TPic pic, int px, int py, int level)
{
    if (pic.pic == nullptr) return;
    if (level == 0) level = 10;
    double sc = level / 20.0 + 0.5;
    int xs = static_cast<int>(pic.x * sc);
    int ys = static_cast<int>(pic.y * sc);
    xs = px - xs;
    ys = py - ys;
    int w = static_cast<int>(pic.pic->w * sc);
    int h = static_cast<int>(pic.pic->h * sc);
    SDL_Surface* scaled = rotozoomSurfaceXY(pic.pic, 0, sc, sc, 0);
    if (!scaled) return;
    int bpp = 4;
    for (int yy = 0; yy < h && yy + ys < screen->h; yy++)
        for (int xx = 0; xx < w && xx + xs < screen->w; xx++)
        {
            if (xx + xs < 0 || yy + ys < 0) continue;
            uint32 pix0 = getpixel(scaled, xx, yy);
            if (pic.black != 0)
            {
                if ((pix0 & 0xFFFFFF) == 0) continue;
                uint32 pix = getpixel(screen, xx + xs, yy + ys);
                uint8_t p01 = pix0 & 0xFF, p02 = (pix0 >> 8) & 0xFF, p03 = (pix0 >> 16) & 0xFF;
                uint8_t p1 = pix & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = (pix >> 16) & 0xFF, p4 = (pix >> 24) & 0xFF;
                p1 = p1 + p01 - (p01 * p1) / 255;
                p2 = p2 + p02 - (p02 * p2) / 255;
                p3 = p3 + p03 - (p03 * p3) / 255;
                putpixel(screen, xx + xs, yy + ys, p1 | (p2 << 8) | (p3 << 16) | (p4 << 24));
            }
            else
            {
                if ((pix0 & 0xFF000000) == 0) continue;
                uint32 pix = getpixel(screen, xx + xs, yy + ys);
                uint8_t p01 = pix0 & 0xFF, p02 = (pix0 >> 8) & 0xFF, p03 = (pix0 >> 16) & 0xFF, p04 = (pix0 >> 24) & 0xFF;
                uint8_t p1 = pix & 0xFF, p2 = (pix >> 8) & 0xFF, p3 = (pix >> 16) & 0xFF, p4 = (pix >> 24) & 0xFF;
                p1 = static_cast<uint8_t>((p04 * p01 + (255 - p04) * p1) / 255);
                p2 = static_cast<uint8_t>((p04 * p02 + (255 - p04) * p2) / 255);
                p3 = static_cast<uint8_t>((p04 * p03 + (255 - p04) * p3) / 255);
                putpixel(screen, xx + xs, yy + ys, p1 | (p2 << 8) | (p3 << 16) | (p4 << 24));
            }
        }
    SDL_DestroySurface(scaled);
}

void PlayBeginningMovie(int beginnum, int endnum)
{
    PlayMP3(106, 1);
    std::string path = AppPath + MOVIE_file;
    FILE* fp = nullptr;
    fopen_s(&fp, path.c_str(), "rb");
    if (!fp) return;
    int count = 0;
    fread(&count, 4, 1, fp);
    if (beginnum < 0) beginnum = count - 1;
    if (endnum < 0) endnum = count - 1;
    if (beginnum > count - 1) beginnum = count - 1;
    if (endnum > count - 1) endnum = count - 1;
    int step = (endnum >= beginnum) ? 1 : -1;
    for (int i = beginnum; step > 0 ? i <= endnum : i >= endnum; i += step)
    {
        while (SDL_PollEvent(&event))
        {
            CheckBasicEvent();
            if (event.type == SDL_EVENT_KEY_UP && (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE))
            { fclose(fp); event.key.key = 0; return; }
        }
        TPic mov = GetPngPic(fp, i);
        ZoomPic(mov.pic, 0, 0, 0, screen->w, screen->h);
        SDL_Delay(20);
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_DestroySurface(mov.pic);
    }
    fclose(fp);
}

void ZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h)
{
    if (!scr) return;
    double zoomx = (w > 0 && scr->w > 0) ? static_cast<double>(w) / scr->w : 1.0;
    double zoomy = (h > 0 && scr->h > 0) ? static_cast<double>(h) / scr->h : 1.0;
    SDL_Surface* zoomed = rotozoomSurfaceXY(scr, angle, zoomx, zoomy, SMOOTH);
    if (zoomed)
    {
        SDL_Rect dest{ x, y, 0, 0 };
        SDL_BlitSurface(zoomed, nullptr, screen, &dest);
        SDL_DestroySurface(zoomed);
    }
}

SDL_Surface* GetZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h)
{
    if (!scr) return nullptr;
    double zoomx = (w > 0 && scr->w > 0) ? static_cast<double>(w) / scr->w : 1.0;
    double zoomy = (h > 0 && scr->h > 0) ? static_cast<double>(h) / scr->h : 1.0;
    return rotozoomSurfaceXY(scr, angle, zoomx, zoomy, SMOOTH);
}

void NewMenuTeammate()
{
    // Simplified: just show teammate status
    SelectShowStatus();
}

void ShowTeammateMenu(int TeamListNum, int RoleListNum, int16_t* rlist, int MaxCount, int position)
{
    display_imgFromSurface(TEAMMATE_PIC.pic, 0, 0);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

void NewMenuItem()
{
    int menu = 0;
    showNewItemMenu(menu);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            int max = 0; for (auto& il : RItemList) if (il.Amount > 0) max++;
            if (max == 0) break;
            if (menu < 0) menu = max - 1; if (menu >= max) menu = 0;
            showNewItemMenu(menu);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) break;
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) break;
        }
    }
}

void showNewItemMenu(int menu)
{
    display_imgFromSurface(MENUITEM_PIC.pic, 0, 0);
    int y_ = 30;
    int count = 0;
    for (auto& il : RItemList)
    {
        if (il.Amount <= 0) continue;
        std::wstring name = GBKToUnicode(RItem[il.Number].Name);
        std::wstring amt = std::to_wstring(il.Amount);
        uint32 c1 = (count == menu) ? ColColor(0, 0x64) : ColColor(0, 5);
        uint32 c2 = (count == menu) ? ColColor(0, 0x66) : ColColor(0, 7);
        DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), 20, y_, c1, c2);
        DrawEngShadowText(reinterpret_cast<uint16_t*>(amt.data()), 200, y_, c1, c2);
        y_ += 22;
        count++;
        if (count > 15) break;
    }
    if (menu >= 0 && menu < static_cast<int>(RItemList.size()) && RItemList[menu].Amount > 0)
        DrawItemPic(RItemList[menu].Number, 400, 100);
    SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
}

int16_t SelectItemUser(int inum)
{
    int menu = 0, max = 0;
    int16_t p[6] = {};
    for (int i = 0; i < 6; i++) if (TeamList[i] >= 0) { p[max] = TeamList[i]; max++; }
    if (max == 0) return -1;
    showSelectItemUser(300, 100, inum, menu, max, p);
    while (SDL_WaitEvent(&event))
    {
        CheckBasicEvent();
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) menu--;
            if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) menu++;
            if (menu < 0) menu = max - 1; if (menu >= max) menu = 0;
            showSelectItemUser(300, 100, inum, menu, max, p);
        }
        else if (event.type == SDL_EVENT_KEY_UP)
        {
            if (event.key.key == SDLK_ESCAPE) return -1;
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) return p[menu];
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            if (event.button.button == SDL_BUTTON_RIGHT) return -1;
            if (event.button.button == SDL_BUTTON_LEFT) return p[menu];
        }
    }
    return -1;
}

void showSelectItemUser(int x, int y, int inum, int menu, int max, int16_t* p)
{
    DrawRectangle(x, y, 150, 10 + max * 22, 0, ColColor(0, 255), 30);
    for (int i = 0; i < max; i++)
    {
        std::wstring name = GBKToUnicode(RRole[p[i]].Name);
        uint32 c1 = (i == menu) ? ColColor(0, 0x64) : ColColor(0, 5);
        uint32 c2 = (i == menu) ? ColColor(0, 0x66) : ColColor(0, 7);
        DrawShadowText(reinterpret_cast<uint16_t*>(name.data()), x + 10, y + 5 + 22 * i, c1, c2);
    }
    SDL_UpdateRect2(screen, x, y, 150, 10 + max * 22);
}

void UpdateBattleScene(int xs, int ys, int oldPic, int newpic)
{
    // Similar to UpdateScene but for battle field - just reinitialize
    InitialWholeBField();
}

void Moveman(int x1, int y1, int x2, int y2)
{
    // Move main character from (x1,y1) to (x2,y2) with animation
    int dx = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
    int dy = (y2 > y1) ? 1 : (y2 < y1 ? -1 : 0);
    int cx = x1, cy = y1;
    while (cx != x2 || cy != y2)
    {
        if (cx != x2) cx += dx;
        if (cy != y2) cy += dy;
        Sx = cx; Sy = cy;
        Redraw();
        SDL_UpdateRect2(screen, 0, 0, screen->w, screen->h);
        SDL_Delay(50);
    }
}

void findway(int x1, int y1)
{
    // Simple pathfinding - just teleport for now
    Mx = x1; My = y1;
}

void DrawCPic(int num, int px, int py, int shadow, int alpha, uint32 mixColor, int mixAlpha)
{
    TRect Area{ 0, 0, screen->w, screen->h };
    if (num >= 0 && num < 20)
        DrawRLE8Pic(num, px, py, CIdx, CPic, Area, nullptr, shadow);
}

void DrawClouds()
{
    for (auto& c : Cloud)
    {
        DrawCPic(c.Picnum, c.Positionx, c.Positiony, c.Shadow, c.Alpha, c.MixColor, c.MixAlpha);
        c.Positionx += c.Speedx;
        c.Positiony += c.Speedy;
        if (c.Positionx > screen->w + 100) c.Positionx = -200;
        if (c.Positionx < -300) c.Positionx = screen->w + 50;
        if (c.Positiony > screen->h + 100) c.Positiony = -200;
        if (c.Positiony < -300) c.Positiony = screen->h + 50;
    }
}

void ChangeCol()
{
    // Cycle palette colors for animation
    uint8_t temp[3];
    std::memcpy(temp, &ACol[252 * 3], 3);
    for (int i = 252; i > 232; i--)
        std::memcpy(&ACol[i * 3], &ACol[(i - 1) * 3], 3);
    std::memcpy(&ACol[232 * 3], temp, 3);
}

void DrawRLE8Pic3(const char* colorPanel, int num, int px, int py, int* Pidx, uint8_t* Ppic,
    const char* RectArea, const char* Image, int widthI, int heightI, int sizeI, int shadow, int alpha,
    const char* BlockImageW, const char* BlockScreenR, int widthR, int heightR, int sizeR, int depth,
    uint32 mixColor, int mixAlpha)
{
    // Advanced RLE8 drawing with alpha blending and depth - simplified implementation
    TRect Area{ 0, 0, screen->w, screen->h };
    DrawRLE8Pic(num, px, py, Pidx, Ppic, Area, Image, shadow, 0, colorPanel);
}
