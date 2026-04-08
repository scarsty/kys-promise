// kys_engine.cpp - 引擎层实现
// 对应 kys_engine.pas 的 implementation 段

#include "kys_engine.h"
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
    // TODO: 实现虚拟按键绘制
}

uint32 CheckBasicEvent()
{
    // TODO: 实现完整的事件检测循环
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_EVENT_QUIT: return SDLK_ESCAPE;
    case SDL_EVENT_KEY_DOWN: return event.key.key;
    }
    return 0;
}

void QuitConfirm()
{
    // TODO: 实现退出确认对话框
}

void resetpallet()
{
    // 默认：复位到默认调色板
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

// ==== 以下函数为桩实现，需后续完善 ====

void DrawSNewPic(int num, int px, int py, int x, int y, int w, int h, int mask) { /* TODO */ }
void InitNewPic(int num, int px, int py, int x, int y, int w, int h) { InitNewPic(num, px, py, x, y, w, h, 0); }
void InitNewPic(int num, int px, int py, int x, int y, int w, int h, int mask) { /* TODO */ }
void DrawHeadPic(int num, int px, int py) { /* TODO */ }
void DrawMMap() { /* TODO */ }
void DrawScene() { /* TODO */ }
void DrawSceneWithoutRole(int x, int y) { /* TODO */ }
void DrawRoleOnScene(int x, int y) { /* TODO */ }
void InitialScene() { /* TODO */ }
void UpdateScene(int xs, int ys, int oldpic, int newpic) { /* TODO */ }
void LoadScenePart(int x, int y) { /* TODO */ }
void DrawWholeBField() { /* TODO */ }
void DrawBfieldWithoutRole(int x, int y) { /* TODO */ }
void DrawRoleOnBfield(int x, int y) { /* TODO */ }
void InitialWholeBField() { /* TODO */ }
void LoadBfieldPart(int x, int y) { /* TODO */ }
void DrawBFieldWithCursor(int AttAreaType, int step, int range) { /* TODO */ }
void DrawBFieldWithEft(int f, int Epicnum, int bigami, int level) { /* TODO */ }
void DrawBFieldWithAction(int f, int bnum, int Apicnum) { /* TODO */ }
void NewMenuSystem() { /* TODO */ }
void SelectShowStatus() { /* TODO */ }
void NewShowStatus(int rnum) { /* TODO */ }
void SelectShowMagic() { /* TODO */ }
void NewShowMagic(int rnum) { /* TODO */ }
void ShowMagic(int rnum, int num, int x1, int y1, int w, int h, bool showit) { /* TODO */ }
bool InModeMagic(int rnum) { return false; /* TODO */ }
void UpdateHpMp(int rnum, int x, int y) { /* TODO */ }
void MenuMedcine(int rnum) { /* TODO */ }
void MenuMedPoision(int rnum) { /* TODO */ }
TPic GetPngPic(const std::string& filename, int num) { return {}; /* TODO */ }
// GetPngPic(FILE*, int) is in kys_main.cpp
void drawPngPic(TPic image, int x, int y, int w, int h, int px, int py, int mask) { /* TODO */ }
void drawPngPic(TPic image, int px, int py, int mask) { drawPngPic(image, 0, 0, 0, 0, px, py, mask); }
SDL_Surface* ReadPicFromByte(uint8_t* p_byte, int size)
{
    auto io = SDL_IOFromMem(p_byte, size);
    return IMG_Load_IO(io, true);
}
std::string Simplified2Traditional(const std::string& mSimplified) { return mSimplified; /* TODO */ }
std::wstring Traditional2Simplified(const std::wstring& mTraditional) { return mTraditional; /* TODO */ }
void NewShowMenuSystem(int menu) { /* TODO */ }
bool NewMenuSave() { return false; /* TODO */ }
void NewShowSelect(int row, int menu, const std::wstring* word, int count, int Width) { /* TODO */ }
bool NewMenuLoad() { return false; /* TODO */ }
void NewMenuVolume() { /* TODO */ }
void NewMenuQuit() { /* TODO */ }
void DrawItemPic(int num, int x, int y) { /* TODO */ }
void ShowMap() { /* TODO */ }
void NewMenuEsc() { /* TODO */ }
void showNewMenuEsc(int menu, int* positionX, int* positionY) { /* TODO */ }
void DrawEftPic(TPic Pic, int px, int py, int level) { /* TODO */ }
void PlayBeginningMovie(int beginnum, int endnum) { /* TODO */ }
void ZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h) { /* TODO */ }
SDL_Surface* GetZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h) { return nullptr; /* TODO */ }
void NewMenuTeammate() { /* TODO */ }
void ShowTeammateMenu(int TeamListNum, int RoleListNum, int16_t* rlist, int MaxCount, int position) { /* TODO */ }
void NewMenuItem() { /* TODO */ }
void showNewItemMenu(int menu) { /* TODO */ }
int16_t SelectItemUser(int inum) { return -1; /* TODO */ }
void showSelectItemUser(int x, int y, int inum, int menu, int max, int16_t* p) { /* TODO */ }
void UpdateBattleScene(int xs, int ys, int oldPic, int newpic) { /* TODO */ }
void Moveman(int x1, int y1, int x2, int y2) { /* TODO */ }
void findway(int x1, int y1) { /* TODO */ }
void DrawCPic(int num, int px, int py, int shadow, int alpha, uint32 mixColor, int mixAlpha) { /* TODO */ }
void DrawClouds() { /* TODO */ }
void ChangeCol() { /* TODO */ }
void DrawRLE8Pic3(const char* colorPanel, int num, int px, int py, int* Pidx, uint8_t* Ppic,
    const char* RectArea, const char* Image, int widthI, int heightI, int sizeI, int shadow, int alpha,
    const char* BlockImageW, const char* BlockScreenR, int widthR, int heightR, int sizeR, int depth,
    uint32 mixColor, int mixAlpha) { /* TODO */ }