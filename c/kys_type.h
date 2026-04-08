#pragma once
// kys_type.h - 类型定义与全局变量声明
// 对应 kys_main.pas 的 interface 段中的 type/var/const

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// ---- 基本类型别名 ----
using uint32 = uint32_t;
using uint16 = uint16_t;

// ---- 结构体定义 ----

struct TPosition
{
    int x = 0, y = 0;
};

struct TPoint
{
    int x = 0, y = 0;
};

struct TRect
{
    int x = 0, y = 0, w = 0, h = 0;
};

struct TPic
{
    int x = 0, y = 0, black = 0;
    SDL_Surface* pic = nullptr;
};

struct TItemList
{
    int16_t Number = 0, Amount = 0;
};

struct TCloud
{
    int Picnum = 0;
    int Shadow = 0;
    int Alpha = 0;
    uint32 MixColor = 0;
    int MixAlpha = 0;
    int Positionx = 0, Positiony = 0, Speedx = 0, Speedy = 0;
};

// ---- 游戏核心数据结构 ----
// 使用 union 实现按元素/按数组两种访问方式，对应Pascal中的variant record

#pragma pack(push, 1)

struct TRole
{
    union
    {
        struct
        {
            int16_t ListNum, HeadNum, IncLife, UnUse;
            char Name[10];
            char Nick[10];
            int16_t Sexual, Level;
            uint16_t Exp;
            int16_t CurrentHP, MaxHP, Hurt, Poision, PhyPower;
            uint16_t ExpForItem;
            int16_t Equip[5];
            int16_t Gongti;
            int16_t TeamState;
            int16_t Angry;
            uint16_t GongtiExam;
            int16_t Moveable, AddSkillPoint, PetAmount;
            int16_t Impression, Reset, difficulty;
            int16_t SoundDealy[2];
            int16_t MPType, CurrentMP, MaxMP;
            int16_t Attack, Speed, Defence, Medcine, UsePoi, MedPoi, DefPoi, Fist, Sword, Knife, Unusual, HidWeapon;
            int16_t Knowledge, Ethics, AttPoi, AttTwice, Repute, Aptitude, PracticeBook;
            uint16_t ExpForBook;
            int16_t Magic[10], MagLevel[10];
            int16_t TakingItem[4], TakingItemAmount[4];
        };
        int16_t Data[91];
    };
};

struct TItem
{
    union
    {
        struct
        {
            int16_t ListNum;
            char Name[20];
            int16_t ExpOfMagic;
            int16_t SetNum, BattleEffect, WineEffect, needSex;
            int16_t unuse[5];
            char Introduction[30];
            int16_t Magic, AmiNum, User, EquipType, ShowIntro, ItemType, inventory, price, EventNum;
            int16_t AddCurrentHP, AddMaxHP, AddPoi, AddPhyPower, ChangeMPType, AddCurrentMP, AddMaxMP;
            int16_t AddAttack, AddSpeed, AddDefence, AddMedcine, AddUsePoi, AddMedPoi, AddDefPoi;
            int16_t AddFist, AddSword, AddKnife, AddUnusual, AddHidWeapon, AddKnowledge, AddEthics;
            int16_t AddAttTwice, AddAttPoi;
            int16_t OnlyPracRole, NeedMPType, NeedMP, NeedAttack, NeedSpeed, NeedUsePoi, NeedMedcine, NeedMedPoi;
            int16_t NeedFist, NeedSword, NeedKnife, NeedUnusual, NeedHidWeapon, NeedAptitude;
            int16_t NeedExp, Count, Rate;
            int16_t NeedItem[5], NeedMatAmount[5];
        };
        int16_t Data[95];
    };
};

struct TScene
{
    union
    {
        struct
        {
            int16_t ListNum;
            char Name[10];
            int16_t ExitMusic, EntranceMusic;
            int16_t Pallet, EnCondition;
            int16_t MainEntranceY1, MainEntranceX1, MainEntranceY2, MainEntranceX2;
            int16_t EntranceY, EntranceX;
            int16_t ExitY[3], ExitX[3];
            int16_t Mapmode, mapnum, useless3, useless4;
        };
        int16_t Data[26];
    };
};

struct TMagic
{
    union
    {
        struct
        {
            int16_t ListNum;
            char Name[10];
            int16_t Useless, NeedHP, MinStep, bigami, EventNum;
            int16_t SoundNum, MagicType, AmiNum, HurtType, AttAreaType, NeedMP, Poision;
            int16_t MinHurt, MaxHurt, HurtModulus, AttackModulus, MPModulus, SpeedModulus, WeaponModulus;
            int16_t NeedProgress, AddMpScale, AddHpScale;
            int16_t MoveDistance[10], AttDistance[10];
            int16_t AddHP[3], AddMP[3], AddAtt[3], AddDef[3], AddSpd[3];
            int16_t MinPeg, MaxPeg, MinInjury, MaxInjury, AddMedcine, AddUsePoi, AddMedPoi, AddDefPoi;
            int16_t AddFist, AddSword, AddKnife, AddUnusual, AddHidWeapon, BattleState;
            int16_t NeedExp[3];
            int16_t MaxLevel;
            char Introduction[60];
        };
        int16_t Data[111];
    };
};

struct TShop
{
    union
    {
        struct
        {
            int16_t Item[18];
        };
        int16_t Data[18];
    };
};

struct TBattleRole
{
    union
    {
        struct
        {
            int16_t rnum, Team, Y, X, Face, Dead, Step, Acted;
            int16_t Pic, ShowNumber, Progress, Round, speed;
            int16_t ExpGot, Auto, Show, wait, frozen, killed, Knowledge, LifeAdd;
            int16_t AddAtt, AddDef, AddSpd, AddStep, AddDodge, PerfectDodge;
        };
        int16_t Data[27];
    };
};

struct TWarSta
{
    union
    {
        struct
        {
            int16_t BattleNum;
            uint8_t BattleName[10];
            int16_t battlemap, exp, battlemusic;
            int16_t mate[12], automate[12], mate_x[12], mate_y[12];
            int16_t enemy[30], enemy_x[30], enemy_y[30];
            int16_t BoutEvent, OperationEvent;
            int16_t GetKongfu[3];
            int16_t GetItems[3];
            int16_t GetMoney;
        };
        int16_t Data[156];
    };
};

#pragma pack(pop)

struct TBRoleColor
{
    int green = 0, red = 0, yellow = 0, blue = 0, gray = 0;
};

struct TStretchInfo
{
    int px = 0, py = 0, num = 0, den = 0;
};

// ---- 函数指针类型 ----
using TPInt1 = void (*)(int);

// ---- 常量 ----
inline constexpr uint32 RMask = 0xFF0000;
inline constexpr uint32 GMask = 0xFF00;
inline constexpr uint32 BMask = 0xFF;
inline constexpr uint32 AMask = 0xFF000000;

// ---- 全局变量 (小变量 inline, 大数组 extern) ----

// 程序路径
inline std::string AppPath;
inline std::string versionstr = "Promise";

// 字体设置
inline int HW = 0;
inline const char* CHINESE_FONT = "resource/Chinese.ttf";
inline int CHINESE_FONT_SIZE = 20;
inline const char* ENGLISH_FONT = "resource/English.ttf";
inline int ENGLISH_FONT_SIZE = 18;
inline int CENTER_X = 320;
inline int CENTER_Y = 220;

// 文件名
inline const char* KDEF_IDX = "resource/kdef.idx";
inline const char* KDEF_GRP = "resource/kdef.grp";
inline const char* TALK_IDX = "resource/talk.idx";
inline const char* TALK_GRP = "resource/talk.grp";
inline const char* NAME_IDX = "resource/name.idx";
inline const char* NAME_GRP = "resource/name.grp";
inline const char* ITEMS_file = "resource/items.Pic";
inline const char* HEADS_file = "resource/heads.Pic";
inline const char* BackGround_file = "resource/BackGround.Pic";
inline const char* GAME_file = "resource/Game.Pic";
inline const char* MOVIE_file = "resource/Begin.pic";
inline const char* Scene_file = "resource/Scene.pic";
inline const char* Skill_file = "resource/Skill.pic";

// 游戏常数
inline int AutoRefresh = 0;
inline int ITEM_BEGIN_PIC = 3501;
inline int BEGIN_EVENT = 691;
inline int BEGIN_Scene = 70;
inline int BEGIN_Sx = 20;
inline int BEGIN_Sy = 19;
inline int SOFTSTAR_BEGIN_TALK = 2547;
inline int SOFTSTAR_NUM_TALK = 18;
inline int MAX_PHYSICAL_POWER = 100;
inline int MONEY_ID = 0;
inline int COMPASS_ID = 1;
inline int MAP_ID = 303;
inline int BEGIN_LEAVE_EVENT = 950;
inline int BEGIN_BATTLE_ROLE_PIC = 2553;
inline int MAX_LEVEL = 30;
inline int MAX_WEAPON_MATCH = 7;
inline int MIN_KNOWLEDGE = 0;
inline int MAX_ITEM_AMOUNT = 300;
inline int MAX_HP = 999;
inline int MAX_MP = 999;
inline int Showanimation = 0;
// MaxProList[0..15] 对应 Data[43..58]
inline int MaxProList[16] = { 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 100, 100, 100, 1 };
inline int SoundVolume = 32;
inline int LIFE_HURT = 10;
inline int Debug = 0;
inline int BEGIN_WALKPIC = 2500;

// 游戏运行时状态
inline int16_t gametime = 0;
inline TWarSta Warsta = {};

// 贴图数据
inline std::vector<uint8_t> MPic, SPic, WPic;
inline std::vector<int> MIdx, SIdx, WIdx;

// 主地图数据 (extern - 大数组)
extern int16_t Earth[480][480];
extern int16_t Surface[480][480];
extern int16_t Building[480][480];
extern int16_t BuildX[480][480];
extern int16_t BuildY[480][480];
extern int16_t Entrance[480][480];

// 调色板数据
inline uint8_t ACol[769] = {};
inline uint8_t Col[4][768] = {};

// 地图状态变量
inline int16_t InShip = 0, Useless1 = 0, Mx = 0, My = 0, Sx = 0, Sy = 0, MFace = 0;
inline int16_t ShipX = 0, ShipY = 0, ShipFace = 0;
inline int16_t TeamList[6] = {};
inline std::vector<TItemList> RItemList;
inline bool isbattle = false;
inline int MStep = 0, Still = 0;
inline int Cx = 0, Cy = 0, SFace = 0, SStep = 0;
inline int CurScene = 0, CurItem = 0, CurEvent = 0, CurMagic = 0, CurrentBattle = 0, Where = 0;
inline int SaveNum = 0;

// R文件数据
inline std::vector<TRole> RRole;
inline std::vector<TItem> RItem;
inline std::vector<TScene> RScene;
inline std::vector<TMagic> RMagic;
inline std::vector<TShop> RShop;

// 物品列表
inline int16_t ItemList[501] = {};

// S, D 文件数据
// SData[scene][layer][y][x], DData[scene][event][field]
inline std::vector<std::vector<std::array<std::array<int16_t, 64>, 64>>> SData;
inline std::vector<std::vector<std::array<int16_t, 11>>> DData;

// 场景图形映像
extern uint32_t SceneImg[2304][1152];
extern uint8_t MaskArray[2304][1152];
inline std::vector<TPic> ScenePic;
inline SDL_Surface* build = nullptr;

// 战场图形映像
extern uint32_t BFieldImg[2304][1152];

// 战场数据
extern int16_t BField[8][64][64];
inline TBattleRole BRole[42] = {};
inline int BRoleAmount = 0;
inline int Bx = 0, By = 0, Ax = 0, Ay = 0;
inline int Bstatus = 0;
inline int maxspeed = 0;

// 升级列表
inline int16_t LevelUpList[100] = {};

// 天气
inline int Water = 0;
inline int snow = 0;
inline int rain = 0;
inline bool fog = false;
inline bool showblackscreen = false;
inline uint8_t snowalpha[440][640] = {};
inline int BattleMode = 0;
inline int fullscreen = 0;
inline int Simple = 0;

// UI 图片资源
inline TPic STATE_PIC = {}, BEGIN_PIC = {}, MAGIC_PIC = {}, SYSTEM_PIC = {}, MAP_PIC = {}, SKILL_PIC = {};
inline TPic MENUITEM_PIC = {}, MENUESC_PIC = {}, MenuescBack_PIC = {}, battlepic = {}, TEAMMATE_PIC = {};
inline TPic PROGRESS_PIC = {}, MATESIGN_PIC = {}, SELECTEDMATE_PIC = {}, ENEMYSIGN_PIC = {}, SELECTEDENEMY_PIC = {}, DEATH_PIC = {};
inline TPic NowPROGRESS_PIC = {}, angryprogress_pic = {}, angrycollect_pic = {}, angryfull_pic = {}, Maker_Pic = {};
inline std::vector<TPic> Head_Pic;
inline std::vector<TPic> SkillPIC;

// 小游戏
inline std::vector<TPosition> snake;
inline int RANX = 0, RANY = 0;
inline int dest = 0;

// 物品图片
inline std::vector<TPic> ITEM_PIC;

// 主画面
inline SDL_Surface* screen = nullptr;
inline SDL_Surface* prescreen = nullptr;
inline SDL_Surface* realscreen = nullptr;
inline SDL_Surface* freshscreen = nullptr;
inline SDL_Event event = {};
inline TTF_Font* Font = nullptr;
inline TTF_Font* EngFont = nullptr;
inline SDL_Color TextColor = {};
inline SDL_Surface* TextSurface = nullptr;
inline int ExitSceneMusicNum = 0;
inline std::string MusicName;

// 选单字符串
inline std::vector<std::wstring> MenuString;
inline std::vector<std::wstring> MenuEngString;

// 扩展指令变量 x50[-0x8000..0x7FFF]
// C++中用 x50[i + 0x8000] 访问
extern int16_t x50[0x10000];

// 游戏状态变量
inline std::vector<std::vector<int>> GameArray;
inline int GameSpeed = 10;
inline int MusicVolume = 64;
inline MIX_Audio* Music[110] = {};
inline MIX_Audio* ESound[187] = {};
inline MIX_Audio* ASound[100] = {};
inline int nowmusic = 0;

// 战斗变量
inline int CurBRole = 0;
inline bool ShowMR = true;
inline uint32 now2 = 0;
inline int16_t time_ = -1; // 'time' 是C标准库函数名，加下划线
inline int16_t timeevent = -1;
inline int rs = 0;
inline int16_t RandomEvent = 0;
inline int randomcount = 0;
inline int Effect = 0;
inline bool HighLight = false;

// 暂存主角武功
inline int16_t magicTemp[10] = {};
inline int16_t magicLvTemp[10] = {};

// 色彩调整
inline int green = 0;
inline int red = 0;
inline int yellow = 0;
inline int blue = 0;
inline int gray = 0;

// 寻路
extern int16_t FWay[480][480];
extern int16_t linex[480 * 480];
extern int16_t liney[480 * 480];
inline int nowstep = 0;
// SetNum[1..5][0..3] => C++: SetNum[0..4][0..3], 使用时 SetNum[i-1][j]
inline int16_t SetNum[5][4] = {};

// 区域
inline TRect RegionRect = {};

// 云
inline int CLOUD_AMOUNT = 60;
inline std::vector<TCloud> Cloud;
inline uint8_t CPic[100001] = {};
inline int CIdx[21] = {};

// 渲染设置
inline int GLHR = 1;
inline int SMOOTH = 1;
inline uint32 ScreenFlag = 0;
inline int RESOLUTIONX = 0;
inline int RESOLUTIONY = 0;
inline SDL_Window* window = nullptr;
inline SDL_Renderer* render = nullptr;
inline SDL_Texture* screenTex = nullptr;

// 手机模式
inline int CellPhone = 0;
inline int ScreenRotate = 0;
inline int KEEP_SCREEN_RATIO = 1;
inline int FingerCount = 0;
inline uint32 FingerTick = 0;

// 虚拟按键
inline SDL_Surface* VirtualKeyU = nullptr;
inline SDL_Surface* VirtualKeyD = nullptr;
inline SDL_Surface* VirtualKeyL = nullptr;
inline SDL_Surface* VirtualKeyR = nullptr;
inline SDL_Surface* VirtualKeyA = nullptr;
inline SDL_Surface* VirtualKeyB = nullptr;
inline int ShowVirtualKey = 0;
inline uint32 VirtualKeyValue = 0;
inline int VirtualKeyX = 150;
inline int VirtualKeyY = 250;
inline int VirtualKeySize = 60;
inline int EXIT_GAME = 1;
inline bool AskingQuit = false;
inline SDL_Surface* virtualKeyScr = nullptr;
