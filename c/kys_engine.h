#pragma once
// kys_engine.h - 引擎层：音频、绘图、文字、场景渲染
// 对应 kys_engine.pas 的 interface 段

#include "kys_type.h"

#include <string>
#include <cstdio>

// ---- 事件过滤 ----
bool EventFilter(void* p, SDL_Event* e);

// ---- 音频 ----
void InitialMusic();
void PlayMP3(int MusicNum, int times, int frombeginning = 1);
void StopMP3();
void PlaySoundE(int SoundNum, int times);
void PlaySoundE(int SoundNum);
void PlaySound(int SoundNum, int times);
void PlaySoundEffect(int SoundNum, int times = 1);

// ---- 基本绘图 ----
uint32 getpixel(SDL_Surface* surface, int x, int y);
void putpixel(SDL_Surface* surface_, int x, int y, uint32 pixel);
void drawscreenpixel(int x, int y, uint32 color);
void display_bmp(const char* file_name, int x, int y);
void display_img(const char* file_name, int x, int y);
void display_img(const char* file_name, int x, int y, int x1, int y1, int w, int h);
uint32 ColColor(int num);
uint32 ColColor(int colnum, int num);
void DrawLine(int x1, int y1, int x2, int y2, int color, int Width);

// ---- RLE8 图片绘制 ----
bool JudgeInScreen(int px, int py, int w, int h, int xs, int ys);
bool JudgeInScreen(int px, int py, int w, int h, int xs, int ys, int xx, int yy, int xw, int yh);
void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea, const char* Image, int Shadow);
void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea, const char* Image, int Shadow, int mask);
void DrawRLE8Pic(int num, int px, int py, int* Pidx, uint8_t* Ppic, TRect RectArea, const char* Image, int Shadow, int mask, const char* colorPanel);
TPosition GetPositionOnScreen(int x, int y, int CenterX, int CenterY);
void DrawTitlePic(int imgnum, int px, int py);
void DrawMPic(int num, int px, int py, int mask);
void DrawSPic(int num, int px, int py, int x, int y, int w, int h);
void DrawSPic(int num, int px, int py, int x, int y, int w, int h, int mask);
void DrawSNewPic(int num, int px, int py, int x, int y, int w, int h, int mask);
void InitialSPic(int num, int px, int py, int x, int y, int w, int h);
void InitialSPic(int num, int px, int py, int x, int y, int w, int h, int mask);
void DrawHeadPic(int num, int px, int py);
void DrawBPic(int num, int px, int py, int shadow);
void DrawBPic(int num, int px, int py, int shadow, int mask);
void DrawBPic(int num, int x, int y, int w, int h, int px, int py, int shadow);
void DrawBPic(int num, int x, int y, int w, int h, int px, int py, int shadow, int mask);
void DrawBPicInRect(int num, int px, int py, int shadow, int x, int y, int w, int h);
void InitialBPic(int num, int px, int py);
void InitialBPic(int num, int px, int py, int x, int y, int w, int h, int mask);
void DrawBRolePic(int num, int px, int py, int shadow, int mask);
void DrawBRolePic(int num, int x, int y, int w, int h, int px, int py, int shadow, int mask);

// ---- 文字显示 ----
std::wstring Big5ToUnicode(const char* str);
std::wstring GBKToUnicode(const char* str);
std::string UnicodeToBig5(const wchar_t* str);
std::string UnicodeToGBK(const wchar_t* str);
void DrawText_(SDL_Surface* sur, uint16_t* word, int x_pos, int y_pos, uint32 color);
void DrawEngText(SDL_Surface* sur, uint16_t* word, int x_pos, int y_pos, uint32 color);
void DrawShadowText(uint16_t* word, int x_pos, int y_pos, uint32 color1, uint32 color2);
void DrawEngShadowText(uint16_t* word, int x_pos, int y_pos, uint32 color1, uint32 color2);
void DrawBig5Text(SDL_Surface* sur, const char* str, int x_pos, int y_pos, uint32 color);
void DrawBig5ShadowText(const char* word, int x_pos, int y_pos, uint32 color1, uint32 color2);
void DrawGBKText(SDL_Surface* sur, const char* str, int x_pos, int y_pos, uint32 color);
void DrawGBKShadowText(const char* word, int x_pos, int y_pos, uint32 color1, uint32 color2);
void DrawTextWithRect(uint16_t* word, int x, int y, int w, uint32 color1, uint32 color2);
void DrawRectangle(int x, int y, int w, int h, uint32 colorin, uint32 colorframe, int alpha);
void DrawRectangleWithoutFrame(int x, int y, int w, int h, uint32 colorin, int alpha);

// ---- 绘制整个屏幕 ----
void Redraw();
void DrawMMap();
void DrawScene();
void DrawSceneWithoutRole(int x, int y);
void DrawRoleOnScene(int x, int y);
void InitialScene();
void UpdateScene(int xs, int ys, int oldpic, int newpic);
void LoadScenePart(int x, int y);
void DrawWholeBField();
void DrawBfieldWithoutRole(int x, int y);
void DrawRoleOnBfield(int x, int y);
void InitialWholeBField();
void LoadBfieldPart(int x, int y);
void DrawBFieldWithCursor(int AttAreaType, int step, int range);
void DrawBFieldWithEft(int f, int Epicnum, int bigami, int level);
void DrawBFieldWithAction(int f, int bnum, int Apicnum);

// ---- KG新增的函数 ----
void InitNewPic(int num, int px, int py, int x, int y, int w, int h);
void InitNewPic(int num, int px, int py, int x, int y, int w, int h, int mask);
void NewMenuSystem();
void SelectShowStatus();
void NewShowStatus(int rnum);
void SelectShowMagic();
void NewShowMagic(int rnum);
void ShowMagic(int rnum, int num, int x1, int y1, int w, int h, bool showit);
void display_imgFromSurface(SDL_Surface* image, int x, int y, int x1, int y1, int w, int h);
void display_imgFromSurface(SDL_Surface* image, int x, int y);
void display_imgFromSurface(TPic image, int x, int y, int x1, int y1, int w, int h);
void display_imgFromSurface(TPic image, int x, int y);
bool InModeMagic(int rnum);
void UpdateHpMp(int rnum, int x, int y);
void MenuMedcine(int rnum);
void MenuMedPoision(int rnum);
TPic GetPngPic(const std::string& filename, int num);
TPic GetPngPic(FILE* f, int num);
void drawPngPic(TPic image, int x, int y, int w, int h, int px, int py, int mask);
void drawPngPic(TPic image, int px, int py, int mask);
SDL_Surface* ReadPicFromByte(uint8_t* p_byte, int size);
std::string Simplified2Traditional(const std::string& mSimplified);
std::wstring Traditional2Simplified(const std::wstring& mTraditional);
void NewShowMenuSystem(int menu);
bool NewMenuSave();
void NewShowSelect(int row, int menu, const std::wstring* word, int count, int Width);
bool NewMenuLoad();
void NewMenuVolume();
void NewMenuQuit();
void DrawItemPic(int num, int x, int y);
void ShowMap();
void NewMenuEsc();
void showNewMenuEsc(int menu, int* positionX, int* positionY);
void resetpallet();
void resetpallet(int num);
uint16 RoRforUInt16(uint16 a, uint16 n);
uint16 RoLforUint16(uint16 a, uint16 n);
uint8_t RoRforByte(uint8_t a, uint16 n);
uint8_t RoLforByte(uint8_t a, uint16 n);
void DrawEftPic(TPic Pic, int px, int py, int level);
void PlayBeginningMovie(int beginnum, int endnum);
void ZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h);
SDL_Surface* GetZoomPic(SDL_Surface* scr, double angle, int x, int y, int w, int h);
void NewMenuTeammate();
void ShowTeammateMenu(int TeamListNum, int RoleListNum, int16_t* rlist, int MaxCount, int position);
void NewMenuItem();
void showNewItemMenu(int menu);
int16_t SelectItemUser(int inum);
void showSelectItemUser(int x, int y, int inum, int menu, int max, int16_t* p);
void UpdateBattleScene(int xs, int ys, int oldPic, int newpic);
void Moveman(int x1, int y1, int x2, int y2);
void findway(int x1, int y1);

void DrawCPic(int num, int px, int py, int shadow, int alpha, uint32 mixColor, int mixAlpha);
void DrawClouds();

void ChangeCol();

void DrawRLE8Pic3(const char* colorPanel, int num, int px, int py, int* Pidx, uint8_t* Ppic,
    const char* RectArea, const char* Image, int widthI, int heightI, int sizeI,
    int shadow, int alpha, const char* BlockImageW, const char* BlockScreenR,
    int widthR, int heightR, int sizeR, int depth, uint32 mixColor, int mixAlpha);

void SDL_UpdateRect2(SDL_Surface* surf, int x, int y, int w, int h);
void SDL_GetMouseState2(int& x, int& y);
void ResizeWindow(int w, int h);
void SwitchFullscreen();

bool InRegion(int x, int x0, int x1);

void kyslog(const std::string& formatstring, bool cr = true);

void DrawVirtualKey();
uint32 CheckBasicEvent();
void QuitConfirm();

bool MouseInRegion(int x, int y, int w, int h);
bool MouseInRegion(int x, int y, int w, int h, int& x1, int& y1);
SDL_FRect GetRealRect(int& x, int& y, int& w, int& h, int force = 0);
SDL_FRect GetRealRect(SDL_FRect rect, int force = 0);
TStretchInfo KeepRatioScale(int w1, int h1, int w2, int h2);

// ---- 内部辅助(inline) ----
inline uint8_t BlendByte100(uint8_t src, uint8_t dst, int percent)
{
    return static_cast<uint8_t>((percent * src + (100 - percent) * dst) / 100);
}

inline uint8_t BlendByte255(uint8_t src, uint8_t dst, int alpha)
{
    return static_cast<uint8_t>((alpha * src + (255 - alpha) * dst) / 255);
}

inline void BlendRGB255(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t srcR, uint8_t srcG, uint8_t srcB, int alpha)
{
    r = BlendByte255(srcR, r, alpha);
    g = BlendByte255(srcG, g, alpha);
    b = BlendByte255(srcB, b, alpha);
}