// =====================================================================
//  Retaliation - GTA San Andreas (1.0 US) mod
//  ASI plugin + tiny CLEO 4 script that calls custom opcode 0x1FA5 every
//  frame. All logic lives here; game commands are executed through a
//  private "fake" CRunningScript (same technique plugin-sdk uses).
// =====================================================================
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------------
// Game addresses (GTA SA 1.0 US)
// ---------------------------------------------------------------------
#define ADDR_PROCESS_ONE_COMMAND 0x469EB0
#define ADDR_PED_POOL            0xB74490
#define ADDR_VEH_POOL            0xB74494
#define ADDR_PLAYER_PED          0xB7CD98   // CWorld::Players[0].m_pPed
#define ADDR_TIME_MS             0xB7CB84   // CTimer::m_snTimeInMilliseconds
#define ADDR_ONMISSION           0xA49FC4   // $ONMISSION (global 409)

#define PED_SIZE        0x7C4
#define VEH_SIZE        0xA18
#define PED_HEALTH      0x540
#define PED_VEHICLE     0x58C
#define PED_LASTDAMAGER 0x764
#define PED_TYPE        0x598
#define VEH_FLAGS_2     0x42A               // bit1 = bHasBeenOwnedByPlayer
#define VEH_BLOWN_TIME  0x4D8               // m_nTimeWhenBlowedUp (set when it explodes)
#define VEH_LAST_DAMAGER 0x50C              // m_pLastDamageEntity

#define CUSTOM_OPCODE   0x1FA5
#define PEDTYPE_MISSION1 24

struct CPool { uint8_t* objects; uint8_t* byteMap; int size; int firstFree; };

// ---------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------
static char g_dir[MAX_PATH];
static char g_iniPath[MAX_PATH];
static char g_logPath[MAX_PATH];

static void Log(const char* fmt, ...)
{
    FILE* f = fopen(g_logPath, "a");
    if (!f) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

// ---------------------------------------------------------------------
// Script command runner
// ---------------------------------------------------------------------
typedef char (__thiscall* ProcessOneCommand_t)(void*);
static ProcessOneCommand_t ProcessOneCommand = (ProcessOneCommand_t)ADDR_PROCESS_ONE_COMMAND;

static uint8_t g_script[0x100];
static uint8_t g_code[512];

static int   LV(int i)  { return ((int*)(g_script + 0x3C))[i]; }
static float LVF(int i) { return ((float*)(g_script + 0x3C))[i]; }

struct Cmd {
    int pos, outs;
    explicit Cmd(uint16_t op) : pos(0), outs(0) { memset(g_code, 0, sizeof(g_code)); W16(op); }
    void W8(uint8_t v)   { g_code[pos++] = v; }
    void W16(uint16_t v) { memcpy(g_code + pos, &v, 2); pos += 2; }
    void W32(uint32_t v) { memcpy(g_code + pos, &v, 4); pos += 4; }
    Cmd& I(int v)   { W8(0x01); W32((uint32_t)v); return *this; }
    Cmd& F(float v) { uint32_t u; memcpy(&u, &v, 4); W8(0x06); W32(u); return *this; }
    Cmd& S(const char* s) { int n = (int)strlen(s); W8(0x0E); W8((uint8_t)n); memcpy(g_code + pos, s, n); pos += n; return *this; }
    Cmd& O() { W8(0x03); W16((uint16_t)outs++); return *this; }
    bool Run() {
        memset(g_script, 0, sizeof(g_script));
        memcpy(g_script + 0x08, "RETALI", 7);
        *(uint8_t**)(g_script + 0x10) = g_code;   // base IP
        *(uint8_t**)(g_script + 0x14) = g_code;   // current IP
        g_script[0xC4] = 1;                       // active
        ProcessOneCommand(g_script);
        return g_script[0xC5] != 0;               // condition result
    }
};

// ---------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------
struct TierCfg {
    int models[48]; int nModels;
    int weapons[16]; int nWeapons;
    int cars[16];   int nCars;
    int footPeds, carCount, pedsPerCar, maxActive, intervalSec, accuracy, health, armour;
    float footMin, footMax, carMin, carMax, cruise;
};

static TierCfg g_tier[6];
static int   cfgMaxPoints = 50, cfgKill = 2, cfgTheft = 2, cfgExplode = 4, cfgDecayAmount = 5, cfgDecaySec = 30;
static int   cfgDeathPenalty = 10, cfgCountHunterKills = 0, cfgPauseOnMission = 1;
static int   cfgShowHud = 1, cfgShowBlips = 1, cfgDebugKeys = 1, cfgFirstWaveSec = 20;
static float cfgHudX = 12.0f, cfgHudY = 150.0f, cfgDespawnDist = 380.0f, cfgDismountDist = 30.0f;
static float cfgHudScale = 0.7f;
static int   cfgKeyMenu = VK_F8, cfgKeyDown = VK_LEFT, cfgKeyUp = VK_RIGHT, cfgKeyAmbush = VK_UP, cfgKeyHeli = VK_DOWN;
static int   cfgEventsWhenClosed = 1;
static bool  g_menuOpen = false;
static int   cfgMaxHeatSec = 180, cfgMaxHeatWanted = 6;
static int   g_maxSince = 0; static bool g_lock = false; static int g_heatAnim = 0;
#define HEAT_ANIM_MS 3200
static int   cfgCopPoints = 1, cfgGangPoints = 1, cfgCountMissionPeds = 0;
static int   cfgWantedCap = 1, cfgCapOnMission = 0;
// helicopter
static int   cfgHeliEnabled = 1, cfgHeliModel = 487, cfgHeliWeapon = 29;
static int   cfgHeliAccuracy = 70, cfgHeliHealth = 150, cfgHeliArmour = 100, cfgHeliCooldown = 120;
static int   cfgHeliPeds[16]; static int cfgHeliNPeds = 0;
static float cfgHeliOrbit = 22.0f, cfgHeliHeight = 14.0f;
static int   cfgHeliRappelChance = 50, cfgRappelCount = 4;
static int   cfgHeliChance[6] = { 0, 5, 10, 15, 25, 50 };
static float cfgRappelHeight = 12.0f;
static int   cfgUnitDelaySec = 5, cfgMaxEnemies = 16;
static int   cfgHunterTimeoutSec = 150; static float cfgTimeoutMinDist = 70.0f;
// Hunter attack helicopter
static int   cfgHunterChance = 10, cfgHunterModel = 425, cfgHunterGun = 1, cfgHunterGunDamage = 8;
static int   cfgHunterRockets = 1, cfgHunterRocketSec = 6;
static float cfgHunterOrbit = 45.0f, cfgHunterHeight = 22.0f, cfgHunterSpread = 2.0f;
// red level stars (replace the old HUD)
static int   cfgStars = 1, cfgStarsSameRow = 1, cfgStarR = 200, cfgStarG = 20, cfgStarB = 20, cfgStarBlink = 1;
static float cfgStarsX = 622.5f, cfgStarsY = 82.3f, cfgStarStep = 11.7f, cfgStarRowGap = 15.0f;
static int   cfgStarShadow = 1;
static float cfgStarScaleX = 0.42f, cfgStarScaleY = 1.51f;

static int ParseList(const char* s, int* out, int max)
{
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ' || *s == ',' || *s == '\t') s++;
        if (!*s || *s == ';') break;
        char* end; long v = strtol(s, &end, 10);
        if (end == s) break;
        out[n++] = (int)v; s = end;
    }
    return n;
}

static int   IniInt(const char* sec, const char* key, int def) { return GetPrivateProfileIntA(sec, key, def, g_iniPath); }
static float IniFloat(const char* sec, const char* key, float def)
{
    char buf[64], d[64]; sprintf(d, "%f", def);
    GetPrivateProfileStringA(sec, key, d, buf, sizeof(buf), g_iniPath);
    return (float)atof(buf);
}
static void IniList(const char* sec, const char* key, const char* def, int* out, int max, int* n)
{
    char buf[512];
    GetPrivateProfileStringA(sec, key, def, buf, sizeof(buf), g_iniPath);
    *n = ParseList(buf, out, max);
    if (*n == 0) *n = ParseList(def, out, max);
}

static void LoadConfig()
{
    cfgMaxPoints       = IniInt("Points", "MaxPoints", 50);
    cfgKill            = IniInt("Points", "KillPoints", 2);
    cfgExplode         = IniInt("Points", "ExplodePoints", 4);
    cfgTheft           = IniInt("Points", "TheftPoints", 2);
    cfgDecayAmount     = IniInt("Points", "DecayAmount", 5);
    cfgDecaySec        = IniInt("Points", "DecayIntervalSeconds", 30);
    cfgDeathPenalty    = IniInt("Points", "DeathPenalty", 10);
    cfgCountHunterKills= IniInt("Points", "CountHunterKills", 0);

    cfgPauseOnMission  = IniInt("General", "PauseSpawnsOnMission", 1);
    cfgShowBlips       = IniInt("General", "ShowBlips", 1);
    cfgFirstWaveSec    = IniInt("General", "FirstWaveDelaySeconds", 20);
    cfgDespawnDist     = IniFloat("General", "DespawnDistance", 380.0f);
    cfgUnitDelaySec    = IniInt("General", "SpawnDelaySeconds", 5);
    cfgMaxEnemies      = IniInt("General", "MaxEnemies", 16);
    cfgHunterTimeoutSec= IniInt("General", "HunterTimeoutSeconds", 150);
    cfgTimeoutMinDist  = IniFloat("General", "TimeoutMinDistance", 70.0f);

    cfgHunterChance    = IniInt("Helicopter", "HunterChance", 10);
    cfgHunterModel     = IniInt("Helicopter", "HunterModel", 425);
    cfgHunterGun       = IniInt("Helicopter", "HunterGun", 1);
    cfgHunterGunDamage = IniInt("Helicopter", "HunterGunDamage", 8);
    cfgHunterRockets   = IniInt("Helicopter", "HunterRockets", 1);
    cfgHunterRocketSec = IniInt("Helicopter", "HunterRocketSeconds", 6);
    cfgHunterOrbit     = IniFloat("Helicopter", "HunterOrbitRadius", 45.0f);
    cfgHunterHeight    = IniFloat("Helicopter", "HunterHeight", 22.0f);
    cfgHunterSpread    = IniFloat("Helicopter", "HunterGunSpread", 2.0f);
    if (cfgHunterRocketSec < 2) cfgHunterRocketSec = 2;

    cfgStars           = IniInt("Stars", "Enabled", 1);
    cfgStarsSameRow    = IniInt("Stars", "SameRowAsWanted", 1);
    cfgStarsX          = IniFloat("Stars", "X", 622.5f);
    cfgStarsY          = IniFloat("Stars", "Y", 82.3f);
    cfgStarStep        = IniFloat("Stars", "StarSpacing", 11.7f);
    cfgStarShadow      = IniInt("Stars", "Outline", 1);
    cfgStarRowGap      = IniFloat("Stars", "RowGap", 15.0f);
    cfgStarScaleX      = IniFloat("Stars", "ScaleX", 0.42f);
    cfgStarScaleY      = IniFloat("Stars", "ScaleY", 1.51f);
    cfgStarR           = IniInt("Stars", "R", 200);
    cfgStarG           = IniInt("Stars", "G", 20);
    cfgStarB           = IniInt("Stars", "B", 20);
    cfgStarBlink       = IniInt("Stars", "BlinkOnAmbush", 1);
    cfgDismountDist    = IniFloat("General", "DismountDistance", 30.0f);

    cfgDebugKeys       = IniInt("Debug", "Menu", 0);
    cfgKeyMenu         = IniInt("Menu", "KeyToggleMenu", VK_F8);
    cfgKeyDown         = IniInt("Menu", "KeyLevelDown", VK_LEFT);
    cfgKeyUp           = IniInt("Menu", "KeyLevelUp", VK_RIGHT);
    cfgKeyAmbush       = IniInt("Menu", "KeyForceAmbush", VK_UP);
    cfgKeyHeli         = IniInt("Menu", "KeyForceHeli", VK_DOWN);
    cfgEventsWhenClosed= IniInt("Debug", "ShowEvents", 0);

    cfgCopPoints       = IniInt("Points", "CopKillPoints", 1);
    cfgMaxHeatSec      = IniInt("MaxHeat", "SecondsAtMax", 180);
    cfgMaxHeatWanted   = IniInt("MaxHeat", "WantedLevel", 6);
    cfgGangPoints      = IniInt("Points", "GangKillPoints", 1);
    cfgCountMissionPeds= IniInt("Points", "CountMissionPedKills", 0);

    cfgWantedCap       = IniInt("Wanted", "LimitWantedLevel", 1);
    cfgCapOnMission    = IniInt("Wanted", "LimitDuringMissions", 0);

    cfgHeliEnabled     = IniInt("Helicopter", "Enabled", 1);
    for (int t = 1; t <= 5; t++) {
        static const int def[6] = { 0, 5, 10, 15, 25, 50 };
        char key[32]; sprintf(key, "ChanceTier%d", t);
        cfgHeliChance[t] = IniInt("Helicopter", key, def[t]);
    }
    cfgHeliModel       = IniInt("Helicopter", "Model", 487);
    cfgHeliWeapon      = IniInt("Helicopter", "Weapon", 29);
    cfgHeliAccuracy    = IniInt("Helicopter", "Accuracy", 70);
    cfgHeliHealth      = IniInt("Helicopter", "Health", 150);
    cfgHeliArmour      = IniInt("Helicopter", "Armour", 100);
    cfgHeliCooldown    = IniInt("Helicopter", "CooldownSeconds", 120);
    cfgHeliOrbit       = IniFloat("Helicopter", "OrbitRadius", 22.0f);
    cfgHeliHeight      = IniFloat("Helicopter", "Height", 14.0f);
    IniList("Helicopter", "PedModels", "117,118,120", cfgHeliPeds, 16, &cfgHeliNPeds);
    cfgHeliRappelChance= IniInt("Helicopter", "RappelChance", 50);
    cfgRappelCount     = IniInt("Helicopter", "RappelCount", 4);
    cfgRappelHeight    = IniFloat("Helicopter", "RappelHeight", 12.0f);
    if (cfgRappelCount < 1) cfgRappelCount = 1;
    if (cfgRappelCount > 4) cfgRappelCount = 4;

    cfgShowHud         = IniInt("HUD", "ShowHud", 1);
    cfgHudX            = IniFloat("HUD", "X", 12.0f);
    cfgHudY            = IniFloat("HUD", "Y", 150.0f);
    cfgHudScale        = IniFloat("HUD", "Scale", 0.7f);
    if (cfgHudScale < 0.2f) cfgHudScale = 0.2f;

    // defaults per tier -----------------------------------------------
    // one specific group of NPCs per tier
    struct Def { const char *models, *weapons, *cars; int foot, cars_, perCar, maxA, interval, acc, hp, arm; float fmin, fmax, cmin, cmax, cruise; };
    static const Def defs[6] = {
        { "", "", "", 0,0,0,0,0,0,0,0, 0,0,0,0,0 },
        // 1: casino debt collectors (Leone family)
        { "124,125",         "22,23",          "405,445,421,551,575,576,534,567,461", 2, 1, 2,  4, 120, 30, 100,   0, 70, 95, 210, 270, 30 },
        // 2: casino hitmen (Leone family, other skins)
        { "126,127",         "22,24,25",       "402,429,480,506,533,579,400,412,536,521,522", 2, 1, 3,  6,  90, 45, 100,   0, 70, 95, 210, 270, 35 },
        // 3: rednecks
        { "158,159,160,161,162,200", "25,26,33,24", "543,478,422,554,600,489,500,468,586,463,471", 4, 1, 4, 9, 70, 55, 100, 25, 70, 95, 210, 270, 38 },
        // 4: Russian mafia
        { "111,112,113",     "30,29,28,26",    "579,400,470,489,426,445,402,560,461,581", 4, 2, 3, 12,  55, 65, 120,  50, 70, 95, 210, 280, 42 },
        // 5: Triads (black suits, San Fierro)
        { "117,118,120",     "31,30,27",       "507,560,559,558,561,565,562,445,522,521", 6, 2, 4, 16,  45, 80, 150, 100, 70, 95, 220, 290, 48 },
    };
    for (int t = 1; t <= 5; t++) {
        char sec[16]; sprintf(sec, "Tier%d", t);
        TierCfg& c = g_tier[t];
        IniList(sec, "PedModels", defs[t].models, c.models, 48, &c.nModels);
        IniList(sec, "Weapons",   defs[t].weapons, c.weapons, 16, &c.nWeapons);
        IniList(sec, "Cars",      defs[t].cars, c.cars, 16, &c.nCars);
        c.footPeds    = IniInt(sec, "FootPeds",       defs[t].foot);
        c.carCount    = IniInt(sec, "CarsPerAmbush",  defs[t].cars_);
        c.pedsPerCar  = IniInt(sec, "PedsPerCar",     defs[t].perCar);
        c.maxActive   = IniInt(sec, "MaxActive",      defs[t].maxA);
        c.intervalSec = IniInt(sec, "IntervalSeconds",defs[t].interval);
        c.accuracy    = IniInt(sec, "Accuracy",       defs[t].acc);
        c.health      = IniInt(sec, "Health",         defs[t].hp);
        c.armour      = IniInt(sec, "Armour",         defs[t].arm);
        c.footMin     = IniFloat(sec, "FootMinDist",  defs[t].fmin);
        c.footMax     = IniFloat(sec, "FootMaxDist",  defs[t].fmax);
        c.carMin      = IniFloat(sec, "CarMinDist",   defs[t].cmin);
        c.carMax      = IniFloat(sec, "CarMaxDist",   defs[t].cmax);
        c.cruise      = IniFloat(sec, "CarSpeed",     defs[t].cruise);
        if (c.pedsPerCar < 1) c.pedsPerCar = 1;
        if (c.pedsPerCar > 4) c.pedsPerCar = 4;
        if (c.intervalSec < 5) c.intervalSec = 5;
    }
}

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
static uint32_t g_rng = 12345;
static uint32_t Rand() { g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5; return g_rng; }
static int   RandInt(int lo, int hi) { return hi <= lo ? lo : lo + (int)(Rand() % (uint32_t)(hi - lo + 1)); }
static float RandF(float lo, float hi) { return lo + (hi - lo) * (float)(Rand() % 10001) / 10000.0f; }

static int  Now() { return *(int*)ADDR_TIME_MS; }

static void EntityPos(uint8_t* e, float* x, float* y, float* z)
{
    uint8_t* m = *(uint8_t**)(e + 0x14);
    float* p = m ? (float*)(m + 0x30) : (float*)(e + 0x04);
    *x = p[0]; *y = p[1]; *z = p[2];
}

static float Dist2D(float ax, float ay, float bx, float by) { float dx = ax - bx, dy = ay - by; return sqrtf(dx * dx + dy * dy); }

static bool CharExists(int h) { return Cmd(0x056D).I(h).Run(); }
static bool CharDead(int h)   { return Cmd(0x0118).I(h).Run(); }
static bool CarExists(int h)  { return Cmd(0x056E).I(h).Run(); }
static bool CarDead(int h)    { return Cmd(0x0119).I(h).Run(); }
static bool CharInCar(int h)  { return Cmd(0x00DF).I(h).Run(); }
static void CharPos(int h, float* x, float* y, float* z) { Cmd c(0x00A0); c.I(h).O().O().O().Run(); *x = LVF(0); *y = LVF(1); *z = LVF(2); }
static void CarPos(int h, float* x, float* y, float* z)  { Cmd c(0x00AA); c.I(h).O().O().O().Run(); *x = LVF(0); *y = LVF(1); *z = LVF(2); }

static uint8_t* PoolPtr(int addr, int size, int h)
{
    CPool* pool = *(CPool**)addr;
    int i = h >> 8;
    if (!pool || h < 0 || i >= pool->size || pool->byteMap[i] != (h & 0xFF)) return 0;
    return pool->objects + i * size;
}
static uint8_t* PedPtr(int h) { return PoolPtr(ADDR_PED_POOL, PED_SIZE, h); }
static uint8_t* VehPtr(int h) { return PoolPtr(ADDR_VEH_POOL, VEH_SIZE, h); }

// Native game functions used for the SWAT rope (as called by CHeli::SendDownSwat 0x6C69C0)
typedef int   (__cdecl*    RegisterRope_t)(void* id, int type, float x, float y, float z, int a, int b, int c, void* holder, int time);
typedef void* (__cdecl*    TaskNew_t)(unsigned size);
typedef void* (__thiscall* SwatRopeCtor_t)(void* self, int ropeId, void* heli);
typedef void  (__thiscall* SetTask_t)(void* mgr, void* task, int slot, bool force);
typedef void* (__thiscall* FindSwatPos_t)(void* heli, float* out, int idx);
static RegisterRope_t RegisterRope = (RegisterRope_t)0x556B40;
static TaskNew_t      TaskNew      = (TaskNew_t)0x61A5A0;
static SwatRopeCtor_t SwatRopeCtor = (SwatRopeCtor_t)0x659470;
static SetTask_t      SetTask      = (SetTask_t)0x681AF0;
static FindSwatPos_t  FindSwatPos  = (FindSwatPos_t)0x6C4760;
// CProjectileInfo::AddProjectile(creator, weaponType, CVector pos, float force, CVector* dir, CEntity* victim)
typedef bool  (__cdecl*    AddProjectile_t)(void* creator, int type, float x, float y, float z, float force, float* dir, void* victim);
static AddProjectile_t AddProjectile = (AddProjectile_t)0x737C80;

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------
enum { EV_NONE, EV_KILL, EV_THEFT, EV_DECAY, EV_AMBUSH, EV_DEATH, EV_DEBUG, EV_HELI, EV_HELI_RAPPEL, EV_EXPLODE };

static int  g_points = 0;
static int  g_lastGain = 0, g_lastDecay = 0;
static int  g_lastNow = -1;
static bool g_init = false;
static bool g_deadHandled = false;
static int  g_event = EV_NONE, g_eventTime = 0, g_eventValue = 0;
static int  g_nextWave = 0;
static int  g_prevTier = 0;
static bool g_enabled = true;
static int  g_pauseReason = 0;  // 0 none, 1 mission, 2 interior

// state: 0 riding car, 1 leaving car, 2 attacking on foot, 3 heli gunner, 4 heli pilot, 5 rappelling
struct Hunter { bool used, heli; int ped, blip, squad, state, stateTime, born; void* task; };

// mercenary helicopter
// type: 0 = gunners on the skids, 1 = mercenaries rappel down (SWAT rope)
struct HeliState { int state; int car, blip, loadStart, lastCmd, cooldownUntil; float ang; int models[4]; int nModels; int pedA, pedB;
                   int type, model; bool crashing, exploded; int crashStart, spawnTime, dropped, lastDrop, doneTime, wreckTime;
                   int nextBurst, burstLeft, lastShot, nextRocket; };
static HeliState g_heli;
struct Squad  { bool used; int car, spawnTime, slowSince; bool dismount; };
#define MAX_HUNTERS 40
#define MAX_SQUADS  10
static Hunter g_h[MAX_HUNTERS];
static Squad  g_s[MAX_SQUADS];

static int g_exHunters[128]; static int g_exPos = 0;

// pending wave (waiting for models)
// a wave is a queue of units spawned one after another (foot first, cars after)
struct Unit { bool car; int carModel, n, tries; int model[4], weapon[4]; };
static bool g_pending = false, g_spawning = false, g_waveAnnounced = false;
static int  g_pendTier = 0, g_pendStart = 0;
static Unit g_units[12]; static int g_nUnits = 0, g_unitIdx = 0, g_nextUnit = 0, g_unitGap = 5000;
static int  g_reqModels[48]; static int g_nReq = 0;

// pool tracking
#define MAX_SLOTS 8192
static uint8_t g_pedId[MAX_SLOTS], g_pedCounted[MAX_SLOTS];
static uint8_t g_vehId[MAX_SLOTS], g_vehOwned[MAX_SLOTS];
static uint32_t g_vehBlown[MAX_SLOTS];
static int g_exCars[32]; static int g_exCarPos = 0;
static bool g_heliRequest = false;

static int TierOf(int pts) { int t = pts / 10; return t < 0 ? 0 : (t > 5 ? 5 : t); }

static void SetEvent(int ev, int val) { g_event = ev; g_eventValue = val; g_eventTime = Now(); }

static void AddPoints(int delta, int ev)
{
    if (g_lock && delta > 0) return;          // locked until the police lose CJ (or he dies / is busted)
    int before = g_points;
    g_points += delta;
    if (g_points > cfgMaxPoints) g_points = cfgMaxPoints;
    if (g_points < 0) g_points = 0;
    if (delta > 0) g_lastGain = Now();
    SetEvent(ev, delta);
    Log("[%d] points %d -> %d (ev %d)", Now(), before, g_points, ev);
}

static bool IsHunterHandle(int h)
{
    for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && g_h[i].ped == h) return true;
    for (int i = 0; i < 128; i++) if (g_exHunters[i] == h) return true;
    return false;
}

static bool IsOurVehicle(int h)
{
    for (int i = 0; i < MAX_SQUADS; i++) if (g_s[i].used && g_s[i].car == h) return true;
    for (int i = 0; i < 32; i++) if (g_exCars[i] == h) return true;
    return g_heli.car == h && g_heli.state == 2;
}

static int TotalHunters() { int n = 0; for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used) n++; return n; }
static int ActiveHunters() { int n = 0; for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && !g_h[i].heli) n++; return n; }

static void ReleaseHunter(Hunter& h, bool exists)
{
    if (h.blip) Cmd(0x0164).I(h.blip).Run();
    if (exists) Cmd(0x01C2).I(h.ped).Run();
    g_exHunters[g_exPos] = h.ped; g_exPos = (g_exPos + 1) % 128;
    h.used = false; h.blip = 0;
}

static bool OnScreen(float x, float y, float z);

// delete it right away when the player can't see it (frees memory), otherwise let the game clean it up
static void DespawnHunter(Hunter& h, float x, float y, float z)
{
    if (!OnScreen(x, y, z + 1.0f)) { Cmd(0x009B).I(h.ped).Run(); ReleaseHunter(h, false); }
    else ReleaseHunter(h, true);
}

static void ReleaseSquad(Squad& s)
{
    if (s.car && CarExists(s.car)) Cmd(0x01C3).I(s.car).Run();
    g_exCars[g_exCarPos] = s.car; g_exCarPos = (g_exCarPos + 1) % 32;
    s.used = false; s.car = 0;
}

static void CancelPending()
{
    for (int i = 0; i < g_nReq; i++) Cmd(0x0249).I(g_reqModels[i]).Run();
    g_nReq = 0; g_pending = false; g_spawning = false;
}

static void HeliReleaseModels()
{
    for (int i = 0; i < g_heli.nModels; i++) Cmd(0x0249).I(g_heli.models[i]).Run();
    g_heli.nModels = 0;
}

static void EndHeli(int cooldownMs)
{
    for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && g_h[i].heli) ReleaseHunter(g_h[i], CharExists(g_h[i].ped));
    if (g_heli.blip) Cmd(0x0164).I(g_heli.blip).Run();
    if (g_heli.state == 2 && CarExists(g_heli.car)) Cmd(0x01C3).I(g_heli.car).Run();
    if (g_heli.state == 1) HeliReleaseModels();
    if (g_heli.car) { g_exCars[g_exCarPos] = g_heli.car; g_exCarPos = (g_exCarPos + 1) % 32; }
    g_heli.state = 0; g_heli.car = 0; g_heli.blip = 0;
    g_heli.cooldownUntil = Now() + cooldownMs;
}

static void CleanupAll()
{
    g_heliRequest = false;
    if (g_heli.state) EndHeli(cfgHeliCooldown * 1000);
    for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used) ReleaseHunter(g_h[i], CharExists(g_h[i].ped));
    for (int i = 0; i < MAX_SQUADS; i++) if (g_s[i].used) ReleaseSquad(g_s[i]);
    if (g_pending) CancelPending();
}

static void ResetState()
{
    // called on game load / new game: entity handles are no longer valid
    memset(g_h, 0, sizeof(g_h));
    memset(g_s, 0, sizeof(g_s));
    memset(g_exHunters, 0, sizeof(g_exHunters));
    memset(&g_heli, 0, sizeof(g_heli));
    g_pending = false; g_spawning = false; g_nReq = 0; g_heliRequest = false;
    memset(g_exCars, 0, sizeof(g_exCars));
    g_points = 0; g_lastGain = g_lastDecay = 0; g_prevTier = 0; g_maxSince = 0; g_lock = false; g_heatAnim = 0;
    g_event = EV_NONE; g_init = false; g_deadHandled = false;
}

// ---------------------------------------------------------------------
// Detection: kills and vehicle thefts (pool scanning)
// ---------------------------------------------------------------------
static void ScanKills(bool award)
{
    CPool* pool = *(CPool**)ADDR_PED_POOL;
    uint8_t* player = *(uint8_t**)ADDR_PLAYER_PED;
    if (!pool || !player) return;
    uint8_t* pveh = *(uint8_t**)(player + PED_VEHICLE);
    int n = pool->size; if (n > MAX_SLOTS) n = MAX_SLOTS;
    for (int i = 0; i < n; i++) {
        uint8_t b = pool->byteMap[i];
        if (b & 0x80) { g_pedId[i] = 0xFF; continue; }
        if (g_pedId[i] != b) { g_pedId[i] = b; g_pedCounted[i] = 0; }
        uint8_t* ped = pool->objects + i * PED_SIZE;
        if (ped == player || g_pedCounted[i]) continue;
        float hp = *(float*)(ped + PED_HEALTH);
        if (hp > 0.0f) continue;
        g_pedCounted[i] = 1;
        if (!award) continue;
        uint8_t* dmg = *(uint8_t**)(ped + PED_LASTDAMAGER);
        if (!dmg || !(dmg == player || (pveh && dmg == pveh))) continue;
        int handle = (i << 8) | b;
        if (!cfgCountHunterKills && IsHunterHandle(handle)) continue;
        int pedType = *(int*)(ped + PED_TYPE);
        int pts = cfgKill;
        if (pedType == 6) pts = cfgCopPoints;                                 // police / SWAT / FBI / army
        else if (pedType >= 7 && pedType <= 16) pts = cfgGangPoints;          // gang members
        if (pts <= 0) continue;
        if (pedType >= 24 && pedType <= 31 && !cfgCountMissionPeds) continue; // script/mission peds
        AddPoints(pts, EV_KILL);
    }
}

static void ScanThefts(bool award)
{
    CPool* pool = *(CPool**)ADDR_VEH_POOL;
    uint8_t* player = *(uint8_t**)ADDR_PLAYER_PED;
    if (!pool || !player) return;
    uint8_t* pveh = *(uint8_t**)(player + PED_VEHICLE);
    float px, py, pz; EntityPos(player, &px, &py, &pz);
    int n = pool->size; if (n > MAX_SLOTS) n = MAX_SLOTS;
    for (int i = 0; i < n; i++) {
        uint8_t b = pool->byteMap[i];
        if (b & 0x80) { g_vehId[i] = 0xFF; continue; }
        uint8_t* veh = pool->objects + i * VEH_SIZE;
        uint8_t owned = (veh[VEH_FLAGS_2] >> 1) & 1;
        uint32_t blown = *(uint32_t*)(veh + VEH_BLOWN_TIME);
        if (g_vehId[i] != b) { g_vehId[i] = b; g_vehOwned[i] = owned; g_vehBlown[i] = blown; continue; }
        if (blown && !g_vehBlown[i] && award) {
            // vehicle just exploded: who damaged it last?
            uint8_t* dmg = *(uint8_t**)(veh + VEH_LAST_DAMAGER);
            if (dmg && (dmg == player || (pveh && dmg == pveh)) && veh != pveh && !IsOurVehicle((i << 8) | b))
                AddPoints(cfgExplode, EV_EXPLODE);
        }
        g_vehBlown[i] = blown;
        if (owned && !g_vehOwned[i] && award) {
            float vx, vy, vz; EntityPos(veh, &vx, &vy, &vz);
            if (Dist2D(px, py, vx, vy) < 15.0f) AddPoints(cfgTheft, EV_THEFT);
        }
        g_vehOwned[i] = owned;
    }
}

// ---------------------------------------------------------------------
// Decay
// ---------------------------------------------------------------------
static int DecayRef() { return g_lastGain > g_lastDecay ? g_lastGain : g_lastDecay; }

static void UpdateDecay(int now)
{
    if (g_points <= 0) { g_lastDecay = now; return; }
    if (now - DecayRef() >= cfgDecaySec * 1000) {
        g_lastDecay = now;
        AddPoints(-cfgDecayAmount, EV_DECAY);
    }
}

// ---------------------------------------------------------------------
// Hunters AI upkeep
// ---------------------------------------------------------------------
static void AttackPlayer(int ped, int playerChar) { Cmd(0x05E2).I(ped).I(playerChar).Run(); }

static void UpdateHunters(int now, int playerChar, float px, float py)
{
    // squads (cars)
    for (int s = 0; s < MAX_SQUADS; s++) {
        Squad& q = g_s[s];
        if (!q.used) continue;
        bool any = false;
        for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && g_h[i].squad == s) { any = true; break; }
        if (!any) {
            if (q.car && CarExists(q.car)) {
                float cx, cy, cz; CarPos(q.car, &cx, &cy, &cz);
                uint8_t* pl = *(uint8_t**)ADDR_PLAYER_PED;
                uint8_t* pv = pl ? *(uint8_t**)(pl + PED_VEHICLE) : 0;
                if (VehPtr(q.car) != pv && Dist2D(cx, cy, px, py) > 80.0f && !OnScreen(cx, cy, cz + 1.0f)) {
                    Cmd(0x00A6).I(q.car).Run();
                    g_exCars[g_exCarPos] = q.car; g_exCarPos = (g_exCarPos + 1) % 32;
                    q.used = false; q.car = 0; continue;
                }
            }
            ReleaseSquad(q); continue;
        }
        if (q.dismount) continue;
        if (!CarExists(q.car) || CarDead(q.car)) { q.dismount = true; continue; }
        float cx, cy, cz; CarPos(q.car, &cx, &cy, &cz);
        Cmd sp(0x02E3); sp.I(q.car).O().Run(); float speed = LVF(0);
        if (speed < 2.0f) { if (!q.slowSince) q.slowSince = now; } else q.slowSince = 0;
        bool stuck = q.slowSince && now - q.slowSince > 5000 && now - q.spawnTime > 8000;
        if (Dist2D(cx, cy, px, py) < cfgDismountDist || stuck || now - q.spawnTime > 70000)
            q.dismount = true;
    }

    // peds
    for (int i = 0; i < MAX_HUNTERS; i++) {
        Hunter& h = g_h[i];
        if (!h.used) continue;
        if (!CharExists(h.ped)) { ReleaseHunter(h, false); continue; }
        if (CharDead(h.ped))    { if (h.heli && h.state == 4) continue;   /* dead pilot: UpdateHeli ejects it */
                                  ReleaseHunter(h, true);  continue; }
        float x, y, z; CharPos(h.ped, &x, &y, &z);
        float dist = Dist2D(x, y, px, py);
        // safety: too far from CJ, or taking too long to reach him -> despawn and free the slot
        bool tooFar  = dist > cfgDespawnDist;
        bool tooSlow = !h.heli && h.state != 5 && now - h.born > cfgHunterTimeoutSec * 1000 && dist > cfgTimeoutMinDist;
        if (tooFar || tooSlow) { if (h.heli) ReleaseHunter(h, true); else DespawnHunter(h, x, y, z); continue; }

        if (h.state == 4) continue;               // heli pilot: driven by UpdateHeli
        if (h.state == 5) {                       // sliding down the rope
            uint8_t* p = PedPtr(h.ped);
            uint8_t* intel = p ? *(uint8_t**)(p + 0x47C) : 0;
            void* cur = intel ? *(void**)(intel + 4 + 3 * 4) : 0;   // primary task slot
            if (cur != h.task || now - h.stateTime > 25000) {
                if (p) *(uint32_t*)(p + 0x1C) |= 1;             // collision back on
                AttackPlayer(h.ped, playerChar); h.state = 2; h.stateTime = now;
            }
            continue;
        }
        if (h.state == 3) {                       // heli gunner: keep shooting at the player
            if (now - h.stateTime > 3000) {
                Cmd st(0x062E); st.I(h.ped).I(0x05E2).O().Run();
                if (LV(0) == 7) AttackPlayer(h.ped, playerChar);
                h.stateTime = now;
            }
            continue;
        }
        if (h.state == 0) {                       // riding in squad car
            if (h.squad >= 0 && g_s[h.squad].used && !g_s[h.squad].dismount) continue;
            if (CharInCar(h.ped)) Cmd(0x0633).I(h.ped).Run();
            h.state = 1; h.stateTime = now;
        } else if (h.state == 1) {                // leaving car
            if (!CharInCar(h.ped)) { AttackPlayer(h.ped, playerChar); h.state = 2; h.stateTime = now; }
            else if (now - h.stateTime > 6000) { AttackPlayer(h.ped, playerChar); h.state = 2; h.stateTime = now; }
        } else {                                  // attacking: re-issue if task finished
            if (now - h.stateTime > 4000) {
                Cmd st(0x062E); st.I(h.ped).I(0x05E2).O().Run();
                if (LV(0) == 7) AttackPlayer(h.ped, playerChar);
                h.stateTime = now;
            }
        }
    }
}

// ---------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------
static void AddReq(int model)
{
    for (int i = 0; i < g_nReq; i++) if (g_reqModels[i] == model) return;
    if (g_nReq < 48) { g_reqModels[g_nReq++] = model; Cmd(0x0247).I(model).Run(); }
}

static int WeaponModel(int weapon) { Cmd c(0x0781); c.I(weapon).O().Run(); return LV(0); }

static bool AllLoaded()
{
    for (int i = 0; i < g_nReq; i++) if (!Cmd(0x0248).I(g_reqModels[i]).Run()) return false;
    return true;
}

static bool IsBike(int m)
{
    static const int bikes[] = { 448, 461, 462, 463, 468, 471, 521, 522, 523, 581, 586 };
    for (int b : bikes) if (b == m) return true;
    return false;
}

static void FillUnit(Unit& u, const TierCfg& c)
{
    u.tries = 0;
    for (int k = 0; k < u.n; k++) {
        u.model[k]  = c.models[RandInt(0, c.nModels - 1)];
        u.weapon[k] = c.weapons[RandInt(0, c.nWeapons - 1)];
        AddReq(u.model[k]);
        int wm = WeaponModel(u.weapon[k]); if (wm > 0) AddReq(wm);
    }
    if (u.car) AddReq(u.carModel);
}

// wave = foot units (pairs) first, then cars further away
static void PrepareWave(int tier)
{
    TierCfg& c = g_tier[tier];
    if (c.nModels == 0 || c.nWeapons == 0) return;
    g_nReq = 0; g_nUnits = 0; g_unitIdx = 0; g_pendTier = tier; g_pendStart = Now();
    int foot = c.footPeds;
    while (foot > 0 && g_nUnits < 12) {
        Unit& u = g_units[g_nUnits++];
        u.car = false; u.carModel = -1; u.n = foot >= 2 ? 2 : 1; foot -= u.n;
        FillUnit(u, c);
    }
    for (int k = 0; k < c.carCount && g_nUnits < 12; k++) {
        Unit& u = g_units[g_nUnits++];
        u.car = c.nCars > 0; u.carModel = u.car ? c.cars[RandInt(0, c.nCars - 1)] : -1;
        u.n = u.car ? c.pedsPerCar : 2;
        if (u.car && IsBike(u.carModel) && u.n > 2) u.n = 2;   // motorcycle: rider + pillion
        FillUnit(u, c);
    }
    g_spawning = false; g_waveAnnounced = false;
    g_pending = g_nUnits > 0;
    // second dice: does a helicopter come with this wave?
    if (g_pending && cfgHeliEnabled && g_heli.state == 0 && !g_heliRequest && Now() >= g_heli.cooldownUntil) {
        int roll = RandInt(1, 100);
        if (roll <= cfgHeliChance[tier]) { g_heliRequest = true; Log("[%d] heli roll %d <= %d%%: helicopter joins the wave", Now(), roll, cfgHeliChance[tier]); }
    }
}

static bool OnScreen(float x, float y, float z) { return Cmd(0x00C2).F(x).F(y).F(z).F(2.5f).Run(); }

static bool FindFootSpawn(float px, float py, float pz, float baseAng, float minR, float maxR, float* ox, float* oy, float* oz)
{
    for (int t = 0; t < 12; t++) {
        float ang = baseAng + RandF(-0.6f, 0.6f) + (t > 5 ? RandF(-3.14f, 3.14f) : 0.0f);
        float r = RandF(minR, maxR);
        Cmd c(0x02C0); c.F(px + cosf(ang) * r).F(py + sinf(ang) * r).F(pz).O().O().O().Run();
        float nx = LVF(0), ny = LVF(1), nz = LVF(2);
        float d = Dist2D(nx, ny, px, py);
        if (d < minR * 0.75f || d > maxR * 1.4f || fabsf(nz - pz) > 20.0f) continue;
        if (OnScreen(nx, ny, nz + 1.0f)) continue;
        *ox = nx; *oy = ny; *oz = nz; return true;
    }
    return false;
}

static bool FindCarSpawn(float px, float py, float pz, float minR, float maxR, float* ox, float* oy, float* oz)
{
    for (int t = 0; t < 12; t++) {
        float ang = RandF(-3.14159f, 3.14159f), r = RandF(minR, maxR);
        Cmd c(0x02C1); c.F(px + cosf(ang) * r).F(py + sinf(ang) * r).F(pz).O().O().O().Run();
        float nx = LVF(0), ny = LVF(1), nz = LVF(2);
        float d = Dist2D(nx, ny, px, py);
        if (d < minR * 0.7f || d > maxR * 1.3f || fabsf(nz - pz) > 30.0f) continue;
        if (OnScreen(nx, ny, nz + 1.0f)) continue;
        *ox = nx; *oy = ny; *oz = nz; return true;
    }
    return false;
}

static int FreeHunterSlot() { for (int i = 0; i < MAX_HUNTERS; i++) if (!g_h[i].used) return i; return -1; }
static int FreeSquadSlot()  { for (int i = 0; i < MAX_SQUADS; i++)  if (!g_s[i].used) return i; return -1; }

static Hunter* SetupHunterEx(int ped, int weapon, int accuracy, int health, int armour, int squad, int state, int playerChar, bool heli, bool wantBlip)
{
    int slot = FreeHunterSlot();
    Cmd(0x01B2).I(ped).I(weapon).I(9999).Run();
    Cmd(0x01B9).I(ped).I(weapon).Run();
    Cmd(0x02E2).I(ped).I(accuracy).Run();
    Cmd(0x0223).I(ped).I(health).Run();
    if (armour > 0) Cmd(0x035F).I(ped).I(armour).Run();
    int blip = 0;
    if (cfgShowBlips && wantBlip) {
        Cmd b(0x0187); b.I(ped).O().Run(); blip = LV(0);
        Cmd(0x0165).I(blip).I(0).Run();       // red
    }
    if (state == 2 || state == 3) AttackPlayer(ped, playerChar);
    if (slot < 0) { if (blip) Cmd(0x0164).I(blip).Run(); Cmd(0x01C2).I(ped).Run(); return 0; }
    Hunter& h = g_h[slot];
    h.used = true; h.heli = heli; h.ped = ped; h.blip = blip; h.squad = squad; h.state = state; h.stateTime = Now(); h.born = Now(); h.task = 0;
    return &h;
}

static void SetupHunter(int ped, int weapon, const TierCfg& c, int squad, int state, int playerChar)
{
    SetupHunterEx(ped, weapon, c.accuracy, c.health, c.armour, squad, state, playerChar, false, true);
}

// returns 0 = no spot found (retry), 1 = spawned, 2 = skipped (too many hunters)
static int SpawnUnit(Unit& u, int playerChar, float px, float py, float pz)
{
    TierCfg& c = g_tier[g_pendTier];
    if (ActiveHunters() + u.n > c.maxActive || TotalHunters() + u.n > cfgMaxEnemies) return 3;   // full: wait
    float x, y, z;
    if (u.car) {
        int sq = FreeSquadSlot();
        if (sq < 0) return 2;
        if (!FindCarSpawn(px, py, pz, c.carMin, c.carMax, &x, &y, &z)) return 0;
        Cmd cc(0x00A5); cc.I(u.carModel).F(x).F(y).F(z).O().Run();
        int car = LV(0);
        if (!CarExists(car)) return 0;
        Cmd(0x0175).I(car).F(atan2f(-(px - x), (py - y)) * 57.29578f).Run();
        Cmd mp(0x01EA); mp.I(car).O().Run(); int seats = LV(0);
        Squad& q = g_s[sq];
        q.used = true; q.car = car; q.spawnTime = Now(); q.slowSince = 0; q.dismount = false;
        for (int k = 0; k < u.n; k++) {
            int ped;
            if (k == 0) { Cmd d(0x0129); d.I(car).I(PEDTYPE_MISSION1).I(u.model[k]).O().Run(); ped = LV(0); }
            else if (k - 1 < seats) { Cmd d(0x01C8); d.I(car).I(PEDTYPE_MISSION1).I(u.model[k]).I(k - 1).O().Run(); ped = LV(0); }
            else break;
            if (!CharExists(ped)) continue;
            SetupHunter(ped, u.weapon[k], c, sq, 0, playerChar);
        }
        Cmd(0x00AE).I(car).I(2).Run();                  // avoid cars
        Cmd(0x00AD).I(car).F(c.cruise).Run();
        Cmd(0x00AF).I(car).I(2).Run();                  // MISSION_RAMPLAYER_FARAWAY
        return 1;
    }
    if (!FindFootSpawn(px, py, pz, RandF(-3.14159f, 3.14159f), c.footMin, c.footMax, &x, &y, &z)) return 0;
    for (int k = 0; k < u.n; k++) {
        Cmd cc(0x009A); cc.I(PEDTYPE_MISSION1).I(u.model[k]).F(x + k * 1.5f).F(y).F(z).O().Run();
        int ped = LV(0);                 // save it: the next command overwrites LV(0)
        if (!CharExists(ped)) continue;
        SetupHunter(ped, u.weapon[k], c, -1, 2, playerChar);
    }
    return 1;
}

// ---------------------------------------------------------------------
// Mercenary helicopter (max pressure). One at a time, two variants:
//  type 0: Maverick with pilot + 4 gunners on the skids (2 per side)
//  type 1: Maverick that hovers near the player while mercenaries rappel
//          down on ropes (same rope task the police SWAT uses)
// Killing the pilot makes it fall and explode.
// ---------------------------------------------------------------------
static void HeliAddModel(int m)
{
    for (int i = 0; i < g_heli.nModels; i++) if (g_heli.models[i] == m) return;
    if (g_heli.nModels < 4) { g_heli.models[g_heli.nModels++] = m; Cmd(0x0247).I(m).Run(); }
}

static bool DropRappeller(int idx, int playerChar)
{
    HeliState& H = g_heli;
    uint8_t* heli = VehPtr(H.car);
    if (!heli) return false;
    float* m = *(float**)(heli + 0x14);
    if (!m) return false;
    float off[3] = { 0, 0, 0 };
    FindSwatPos(heli, off, idx);
    // matrix: right(0..2) forward(4..6) up(8..10) pos(12..14)
    float wx = m[12] + m[0] * off[0] + m[4] * off[1] + m[8]  * off[2];
    float wy = m[13] + m[1] * off[0] + m[5] * off[1] + m[9]  * off[2];
    float wz = m[14] + m[2] * off[0] + m[6] * off[1] + m[10] * off[2];
    void* ropeId = heli + idx;
    int rope = RegisterRope(ropeId, 8, wx, wy, wz, 0, 0, 0, 0, 20000);
    if (rope < 0) { Log("rope %d failed", idx); return false; }

    Cmd cc(0x009A); cc.I(PEDTYPE_MISSION1).I(idx & 1 ? H.pedB : H.pedA).F(wx).F(wy).F(wz).O().Run();
    int ped = LV(0);
    if (!CharExists(ped)) return false;
    Hunter* h = SetupHunterEx(ped, cfgHeliWeapon, cfgHeliAccuracy, cfgHeliHealth, cfgHeliArmour, -1, 5, playerChar, false, true);
    uint8_t* p = PedPtr(ped);
    if (!p || !h) return false;
    *(uint32_t*)(p + 0x1C) &= ~1u;                                  // no collision while on the rope
    void* t = TaskNew(0x1C);
    if (!t) { *(uint32_t*)(p + 0x1C) |= 1; AttackPlayer(ped, playerChar); h->state = 2; return true; }
    SwatRopeCtor(t, (int)(intptr_t)ropeId, heli);
    uint8_t* intel = *(uint8_t**)(p + 0x47C);
    SetTask(intel + 4, t, 3, false);
    heli[0x9B9] = (uint8_t)idx;                 // swat occupants left (same as SendDownSwat)
    heli[0x9BA + idx] = 0xAA;                   // swat state for this rope
    h->task = t; h->stateTime = Now();
    return true;
}

static void HeliRemoveWreck()
{
    HeliState& H = g_heli;
    for (int i = 0; i < MAX_HUNTERS; i++) {
        Hunter& h = g_h[i];
        if (!h.used) continue;
        if (h.state == 5) {                       // still on a rope: drop the rope task first
            uint8_t* p = PedPtr(h.ped);
            if (p) *(uint32_t*)(p + 0x1C) |= 1;
            Cmd pc(0x01F5); pc.I(0).O().Run(); int playerChar = LV(0);
            AttackPlayer(h.ped, playerChar); h.state = 2; h.stateTime = Now();
        }
        if (h.heli && h.state == 3 && CharExists(h.ped)) Cmd(0x0465).I(h.ped).Run();   // detach gunner
        if (h.heli) ReleaseHunter(h, CharExists(h.ped));
    }
    int car = H.car;
    if (CarExists(car)) Cmd(0x00A6).I(car).Run();                                     // DELETE_CAR
    Log("[%d] helicopter wreck removed", Now());
    EndHeli(cfgHeliCooldown * 1000);
}

static int HeliPilotIdx()
{
    for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && g_h[i].heli && g_h[i].state == 4) return i;
    return -1;
}

static void EjectDeadPilot(int idx)
{
    HeliState& H = g_heli;
    Hunter& h = g_h[idx];
    int model = H.pedA;
    uint8_t* heli = VehPtr(H.car);
    float* m = heli ? *(float**)(heli + 0x14) : 0;
    // body first (its model is still in use by the pilot, so it is loaded)
    if (m && Cmd(0x0248).I(model).Run()) {
        float bx = m[12] - m[0] * 2.3f - m[8] * 0.6f;
        float by = m[13] - m[1] * 2.3f - m[9] * 0.6f;
        float bz = m[14] - m[2] * 2.3f - m[10] * 0.6f;
        Cmd bc(0x009A); bc.I(PEDTYPE_MISSION1).I(model).F(bx).F(by).F(bz).O().Run();
        int body = LV(0);
        if (CharExists(body)) {
            g_exHunters[g_exPos] = body; g_exPos = (g_exPos + 1) % 128;
            Cmd(0x0223).I(body).I(0).Run();      // dead, falls out of the heli
            Cmd(0x01C2).I(body).Run();
        }
    }
    if (CharExists(h.ped)) Cmd(0x009B).I(h.ped).Run();     // DELETE_CHAR also clears the driver seat
    ReleaseHunter(h, false);
    Log("[%d] heli pilot ejected", Now());
}

static bool HeliPilotAlive()
{
    for (int i = 0; i < MAX_HUNTERS; i++)
        if (g_h[i].used && g_h[i].heli && g_h[i].state == 4) return !CharDead(g_h[i].ped);
    return false;
}

// Hunter: circles CJ, fires cannon bursts and rockets at him
static void HunterAttack(int now, float px, float py, float pz, float hx, float hy, float hz, float d)
{
    HeliState& H = g_heli;
    if (now - H.lastCmd > 1500) {
        H.ang += 0.30f;
        float tx = px + cosf(H.ang) * cfgHunterOrbit, ty = py + sinf(H.ang) * cfgHunterOrbit;
        Cmd(0x04A2).I(H.car).F(tx).F(ty).F(pz + cfgHunterHeight).F(cfgHunterHeight - 6.0f).F(cfgHunterHeight + 12.0f).Run();
        H.lastCmd = now;
    }
    uint8_t* heli = VehPtr(H.car);
    uint8_t* player = *(uint8_t**)ADDR_PLAYER_PED;
    if (!heli || !player || d > 170.0f) return;
    float ex, ey, ez; EntityPos(player, &ex, &ey, &ez); ez += 0.3f;
    float dx = ex - hx, dy = ey - hy, dz = ez - hz;
    float len = sqrtf(dx * dx + dy * dy + dz * dz);
    if (len < 8.0f) return;
    dx /= len; dy /= len; dz /= len;

    if (cfgHunterGun) {                                   // bursts of 12 shots
        if (now >= H.nextBurst) { H.burstLeft = 12; H.nextBurst = now + 3500 + RandInt(0, 1500); }
        if (H.burstLeft > 0 && now - H.lastShot >= 90) {
            float sx = hx + dx * 5.0f, sy = hy + dy * 5.0f, sz = hz + dz * 5.0f - 0.8f;
            float sp = cfgHunterSpread;
            Cmd(0x06BC).F(sx).F(sy).F(sz).F(ex + RandF(-sp, sp)).F(ey + RandF(-sp, sp)).F(ez + RandF(-sp * 0.4f, sp * 0.4f)).I(cfgHunterGunDamage).Run();
            H.burstLeft--; H.lastShot = now;
        }
    }
    if (cfgHunterRockets && now >= H.nextRocket && len > 20.0f) {
        float sx = hx + dx * 7.0f, sy = hy + dy * 7.0f, sz = hz + dz * 7.0f - 1.0f;
        float dir[3] = { dx, dy, dz };
        AddProjectile(heli, 19, sx, sy, sz, 1.0f, dir, 0);        // 19 = rocket
        H.nextRocket = now + cfgHunterRocketSec * 1000 + RandInt(0, 2000);
    }
}

static void UpdateHeli(int now, int playerChar, float px, float py, float pz)
{
    if (!cfgHeliEnabled || cfgHeliNPeds == 0) return;
    HeliState& H = g_heli;
    bool wanted = TierOf(g_points) > 0 && !g_pauseReason;

    if (H.state == 0) {
        if (!g_heliRequest) return;
        if (!wanted) { g_heliRequest = false; return; }
        if (TotalHunters() + 5 > cfgMaxEnemies) return;      // wait for room (pilot + 4)
        g_heliRequest = false;
        H.nModels = 0;
        // ped models kept apart from the (deduplicated) request list
        H.pedA = cfgHeliPeds[RandInt(0, cfgHeliNPeds - 1)];
        H.pedB = cfgHeliPeds[RandInt(0, cfgHeliNPeds - 1)];
        // 3 variants: Hunter (HunterChance %), otherwise rappel / gunners (RappelChance %)
        if (RandInt(1, 100) <= cfgHunterChance) H.type = 2;
        else H.type = RandInt(1, 100) <= cfgHeliRappelChance ? 1 : 0;
        H.model = H.type == 2 ? cfgHunterModel : cfgHeliModel;
        HeliAddModel(H.model);
        HeliAddModel(H.pedA);
        HeliAddModel(H.pedB);
        int wm = WeaponModel(cfgHeliWeapon); if (wm > 0) HeliAddModel(wm);
        H.loadStart = now; H.state = 1;
        return;
    }

    if (H.state == 1) {
        if (!wanted) { EndHeli(0); return; }
        for (int i = 0; i < H.nModels; i++) if (!Cmd(0x0248).I(H.models[i]).Run()) {
            if (now - H.loadStart > 20000) EndHeli(10000);
            return;
        }
        float ang = RandF(-3.14159f, 3.14159f);
        float x = px + cosf(ang) * 170.0f, y = py + sinf(ang) * 170.0f, z = pz + 45.0f;
        Cmd cc(0x00A5); cc.I(H.model).F(x).F(y).F(z).O().Run();
        int car = LV(0);
        if (!CarExists(car)) { EndHeli(10000); return; }
        Cmd(0x0175).I(car).F(atan2f(-(px - x), (py - y)) * 57.29578f).Run();
        Cmd(0x0825).I(car).Run();                                   // rotor at full speed

        Cmd pc(0x0129); pc.I(car).I(PEDTYPE_MISSION1).I(H.pedA).O().Run();
        int pilot = LV(0);                   // save it: the next command overwrites LV(0)
        if (CharExists(pilot)) {
            SetupHunterEx(pilot, cfgHeliWeapon, cfgHeliAccuracy, cfgHeliHealth, cfgHeliArmour, -1, 4, playerChar, true, false);
            // fireproof pilot: "Fire On Driver" sets the driver of every exploding vehicle on fire,
            // which kept the heli wreck re-exploding (same conflict SWAT Rally had)
            Cmd(0x02AB).I(pilot).I(0).I(1).I(0).I(0).I(0).Run();   // SET_CHAR_PROOFS bullet fire expl coll melee
        }

        if (H.type == 0) {
            // gunners: left side (position 1) and right side (position 3)
            static const float off[4][3] = { {-1.15f, 0.55f, -0.25f}, {-1.15f, -0.65f, -0.25f}, {1.15f, 0.55f, -0.25f}, {1.15f, -0.65f, -0.25f} };
            static const int   side[4]   = { 1, 1, 3, 3 };
            for (int k = 0; k < 4; k++) {
                Cmd gc(0x009A); gc.I(PEDTYPE_MISSION1).I(k & 1 ? H.pedB : H.pedA).F(x).F(y).F(z - 3.0f).O().Run();
                int ped = LV(0);
                if (!CharExists(ped) || !CarExists(car)) continue;
                Cmd(0x0464).I(ped).I(car).F(off[k][0]).F(off[k][1]).F(off[k][2]).I(side[k]).F(180.0f).I(cfgHeliWeapon).Run();
                SetupHunterEx(ped, cfgHeliWeapon, cfgHeliAccuracy, cfgHeliHealth, cfgHeliArmour, -1, 3, playerChar, true, false);
            }
            HeliReleaseModels();
        } else if (H.type == 2) {
            HeliReleaseModels();
            H.nextBurst = now + 4000; H.burstLeft = 0; H.lastShot = 0; H.nextRocket = now + 6000;
        }
        // type 1 keeps the ped models loaded until everyone has rappelled down
        H.blip = 0;
        if (cfgShowBlips) { Cmd b(0x0186); b.I(car).O().Run(); H.blip = LV(0); Cmd(0x0165).I(H.blip).I(0).Run(); }
        H.car = car; H.ang = ang; H.lastCmd = 0; H.state = 2; H.crashing = false;
        H.spawnTime = now; H.dropped = 0; H.lastDrop = 0; H.doneTime = 0; H.wreckTime = 0; H.exploded = false;
        SetEvent(H.type == 1 ? EV_HELI_RAPPEL : EV_HELI, 0);
        Log("[%d] mercenary helicopter spawned (type %d)", now, H.type);
        return;
    }

    // ---- state 2: active
    if (!CarExists(H.car)) { EndHeli(cfgHeliCooldown * 1000); return; }
    float hx, hy, hz; CarPos(H.car, &hx, &hy, &hz);

    // The wreck stays under our control: the game kept re-exploding the
    // released heli wreck (v1.04 bug), so we remove it a few seconds after it
    // went down instead of letting it go.
    bool dead = CarDead(H.car);
    if (dead || H.crashing) {
        if (!H.crashing) { H.crashing = true; H.crashStart = now; }
        if (!dead) {
            // falling: blow up on impact (or after a few seconds anyway) - once
            Cmd gz(0x02CE); gz.F(hx).F(hy).F(hz).O().Run(); float ground = LVF(0);
            if (!H.exploded && (hz - ground < 4.0f || now - H.crashStart > 4500)) {
                H.exploded = true;
                Cmd(0x020B).I(H.car).Run();
                Log("[%d] helicopter blown up", now);
            }
            return;
        }
        if (!H.wreckTime) {
            H.wreckTime = now;
            // wreck can't take any more damage -> nothing can blow it up again
            Cmd(0x02AC).I(H.car).I(1).I(1).I(1).I(1).I(1).Run();     // SET_CAR_PROOFS
            Log("[%d] helicopter wrecked", now);
        }
        bool ropesGone = H.type == 0 || H.dropped == 0 || now - H.lastDrop > 21000;   // ropes live 20 s
        if (now - H.wreckTime > 6000 && ropesGone) HeliRemoveWreck();
        return;
    }
    int pi = HeliPilotIdx();
    if (pi < 0 || CharDead(g_h[pi].ped)) {
        // pilot killed: throw his body out (so no mod can set a "driver" on fire
        // when it explodes), then engine off, fire, and let it fall
        if (pi >= 0) EjectDeadPilot(pi);
        H.crashing = true; H.crashStart = now;
        Cmd(0x0918).I(H.car).I(0).Run();
        Cmd(0x0224).I(H.car).I(200).Run();
        if (H.type == 1) HeliReleaseModels();
        Log("[%d] heli pilot killed -> crashing", now);
        return;
    }

    float d = Dist2D(hx, hy, px, py);
    if (d > 450.0f || TierOf(g_points) == 0) { EndHeli(cfgHeliCooldown * 1000); return; }

    if (H.type == 0) {
        bool gunners = false;
        for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used && g_h[i].heli && g_h[i].state == 3) gunners = true;
        if (!gunners) { EndHeli(cfgHeliCooldown * 1000); return; }
        if (now - H.lastCmd > 1500) {
            // orbit around the player so the side gunners keep line of sight
            H.ang += 0.45f;
            float tx = px + cosf(H.ang) * cfgHeliOrbit, ty = py + sinf(H.ang) * cfgHeliOrbit;
            Cmd(0x04A2).I(H.car).F(tx).F(ty).F(pz + cfgHeliHeight).F(cfgHeliHeight - 4.0f).F(cfgHeliHeight + 10.0f).Run();
            H.lastCmd = now;
        }
        return;
    }

    if (H.type == 2) { HunterAttack(now, px, py, pz, hx, hy, hz, d); return; }

    // ---- type 1: rappel
    if (H.dropped >= cfgRappelCount) {
        if (now - H.doneTime > 8000) EndHeli(cfgHeliCooldown * 1000);    // flies away, mercenaries stay
        return;
    }
    if (now - H.lastCmd > 1500) {
        float k = d > 1.0f ? 10.0f / d : 0.0f;        // hover ~10 m from the player, on the heli's side
        float tx = px + (hx - px) * k, ty = py + (hy - py) * k;
        Cmd(0x04A2).I(H.car).F(tx).F(ty).F(pz + cfgRappelHeight).F(cfgRappelHeight - 3.0f).F(cfgRappelHeight + 6.0f).Run();
        H.lastCmd = now;
    }
    Cmd sp(0x02E3); sp.I(H.car).O().Run(); float speed = LVF(0);
    bool inPlace = d < 30.0f && speed < 5.0f && hz - pz < cfgRappelHeight + 12.0f;
    bool late = now - H.spawnTime > 60000;           // never got in place: drop anyway
    if ((inPlace || late) && now - H.lastDrop > 1200) {
        DropRappeller(3 - H.dropped, playerChar);
        H.dropped++; H.lastDrop = now;
        if (H.dropped >= cfgRappelCount) { H.doneTime = now; HeliReleaseModels(); }
    }
}

// ---------------------------------------------------------------------
// Wanted level cap: every 10 retaliation points removes one police star
// (10 -> max 4, 20 -> 3, 30 -> 2, 40 -> 1, 50 -> 0)
// ---------------------------------------------------------------------
static int WantedCap() { int t = TierOf(g_points); return t == 0 ? 6 : 5 - t; }

static void UpdateWanted()
{
    if (!cfgWantedCap || TierOf(g_points) == 0) return;
    if (!cfgCapOnMission && *(int*)ADDR_ONMISSION) return;
    int cap = WantedCap();
    Cmd w(0x01C0); w.I(0).O().Run();
    if (LV(0) <= cap) return;
    if (cap <= 0) Cmd(0x0110).I(0).Run();
    else Cmd(0x010D).I(0).I(cap).Run();
}

// 5 red stars held for SecondsAtMax: retaliation turns into police heat.
// Points go to 0, CJ gets WantedLevel police stars, and no retaliation points
// are earned until the wanted level is gone (or CJ dies / is busted).
static void UpdateMaxHeat(int now)
{
    Cmd w(0x01C0); w.I(0).O().Run(); int wanted = LV(0);
    if (g_heatAnim) {
        // conversion effect running (red stars blink, then turn yellow) -> then the police stars
        if (now - g_heatAnim >= HEAT_ANIM_MS) {
            g_heatAnim = 0;
            Cmd(0x01F0).I(6).Run();                               // make sure 6 stars are allowed
            Cmd(0x010D).I(0).I(cfgMaxHeatWanted).Run();           // ALTER_WANTED_LEVEL
            Log("[%d] max heat: %d police stars given", now, cfgMaxHeatWanted);
        }
        return;
    }
    if (g_lock) {
        if (wanted == 0) { g_lock = false; Log("[%d] retaliation unlocked (police lost CJ)", now); }
        return;
    }
    if (TierOf(g_points) < 5 || cfgMaxHeatSec <= 0) { g_maxSince = 0; return; }
    if (!g_maxSince) { g_maxSince = now; return; }
    if (now - g_maxSince < cfgMaxHeatSec * 1000) return;
    g_maxSince = 0;
    g_points = 0; g_lock = true; g_heatAnim = now;
    Log("[%d] max heat reached: retaliation -> police, locked", now);
}

static void UpdateSpawner(int now, int playerChar, float px, float py, float pz)
{
    int tier = TierOf(g_points);
    if (tier > g_prevTier) {
        int soon = now + (g_prevTier == 0 ? cfgFirstWaveSec * 1000 : 12000);
        if (g_prevTier == 0 || g_nextWave > soon) g_nextWave = soon;
    }
    g_prevTier = tier;

    g_pauseReason = 0;
    if (cfgPauseOnMission && *(int*)ADDR_ONMISSION) g_pauseReason = 1;
    else { Cmd a(0x077E); a.O().Run(); if (LV(0) != 0) g_pauseReason = 2; }

    if (tier == 0) { if (g_pending) CancelPending(); return; }
    if (g_pauseReason) { if (g_pending) CancelPending(); if (g_nextWave < now + 5000) g_nextWave = now + 5000; return; }

    if (g_pending && g_spawning && now >= g_nextWave) {
        Log("[%d] next ambush due: %d unit(s) of the previous one dropped", now, g_nUnits - g_unitIdx);
        CancelPending();
    }
    if (g_pending) {
        if (!g_spawning) {
            if (AllLoaded()) { g_spawning = true; g_nextUnit = now; }
            else if (now - g_pendStart > 15000) { CancelPending(); g_nextWave = now + 5000; }
            return;
        }
        if (now < g_nextUnit) return;
        Unit& u = g_units[g_unitIdx];
        int r = SpawnUnit(u, playerChar, px, py, pz);
        if (r == 3) { g_nextUnit = now + 3000; return; }                    // enemy limit reached, wait
        if (r == 0 && ++u.tries < 4) { g_nextUnit = now + 1500; return; }   // no spot yet, retry
        if (r == 1) {
            if (!g_waveAnnounced) { SetEvent(EV_AMBUSH, g_pendTier); g_waveAnnounced = true; }
            Log("[%d] ambush unit %d/%d spawned (%s, %d peds, tier %d, total %d)", now, g_unitIdx + 1, g_nUnits, u.car ? "car" : "foot", u.n, g_pendTier, TotalHunters());
        }
        g_unitIdx++;
        g_nextUnit = now + g_unitGap;
        if (g_unitIdx >= g_nUnits) CancelPending();          // releases model requests
        return;
    }
    if (now >= g_nextWave) {
        int iv = g_tier[tier].intervalSec * 1000;
        if (ActiveHunters() >= g_tier[tier].maxActive || TotalHunters() >= cfgMaxEnemies) { g_nextWave = now + 5000; return; }
        PrepareWave(tier);
        if (!g_pending) { g_nextWave = now + 5000; return; }
        // spread the units over ~75% of the interval, at least SpawnDelaySeconds apart
        g_unitGap = iv * 3 / 4 / g_nUnits;
        if (g_unitGap < cfgUnitDelaySec * 1000) g_unitGap = cfgUnitDelaySec * 1000;
        g_nextWave = now + RandInt(iv * 9 / 10, iv * 11 / 10);
    }
}

// ---------------------------------------------------------------------
// HUD (script text / rects, GXT keys are added by Retaliation.cs)
// ---------------------------------------------------------------------
static void TextStyle(float sx, float sy, int r, int g, int b)
{
    Cmd(0x033F).F(sx).F(sy).Run();
    Cmd(0x0340).I(r).I(g).I(b).I(255).Run();
    Cmd(0x0349).I(1).Run();
}
static void Text(float x, float y, const char* key)         { Cmd(0x033E).F(x).F(y).S(key).Run(); }
static void TextN(float x, float y, const char* key, int n) { Cmd(0x045A).F(x).F(y).S(key).I(n).Run(); }
static void Rect(float x, float y, float w, float h, int r, int g, int b, int a)
{ Cmd(0x038E).F(x + w * 0.5f).F(y + h * 0.5f).F(w).F(h).I(r).I(g).I(b).I(a).Run(); }

static void DrawEvent(int now, float X, float Y, float S)
{
    if (g_event != EV_NONE && now - g_eventTime < 3500) {
        const char* key = 0; int r = 255, g = 255, b = 255; bool num = false;
        switch (g_event) {
            case EV_KILL:   key = "RETEVK"; r = 255; g = 90;  b = 90;  num = true; break;
            case EV_THEFT:  key = "RETEVC"; r = 255; g = 170; b = 60;  num = true; break;
            case EV_EXPLODE: key = "RETEVE"; r = 255; g = 120; b = 0;  num = true; break;
            case EV_DECAY:  key = "RETEVD"; r = 90;  g = 220; b = 90;  num = true; break;
            case EV_AMBUSH: key = "RETEVA"; r = 255; g = 40;  b = 40;  break;
            case EV_HELI:   key = "RETEVH"; r = 255; g = 40;  b = 40;  break;
            case EV_HELI_RAPPEL: key = "RETEVR"; r = 255; g = 40; b = 40; break;
            case EV_DEATH:  key = "RETEVX"; r = 180; g = 180; b = 255; num = true; break;
            case EV_DEBUG:  key = "RETEVG"; r = 200; g = 200; b = 255; num = true; break;
        }
        if (key) {
            TextStyle(0.26f * S, 1.15f * S, r, g, b);
            int v = g_eventValue < 0 ? -g_eventValue : g_eventValue;
            if (num) TextN(X, Y, key, v); else Text(X, Y, key);
        }
    }
}

// one red star per level, next to (left of) the police wanted stars
static void StarGlyph(float x, float y, int r, int g, int b)
{
    Cmd(0x033F).F(cfgStarScaleX).F(cfgStarScaleY).Run();
    Cmd(0x0340).I(r).I(g).I(b).I(255).Run();
    Cmd(0x0349).I(0).Run();                 // same font as the wanted stars (']' = star)
    Cmd(0x0348).I(1).Run();
    Cmd(0x03E4).I(1).Run();                 // right aligned, like the wanted stars
    Text(x, y, "RETS1");
}

// one red star per level, drawn star by star in the free slots to the left
// of the police wanted stars (same spacing as the yellow ones)
static void DrawStars(int now, bool playing)
{
    if (!cfgStars || !playing) return;
    if (g_heatAnim) {
        // conversion effect: 5 red stars blink, then flip between red and yellow
        int t = now - g_heatAnim;
        bool show = true; bool yellow = false;
        if (t < 2000) show = (t / 150) % 2 == 0;
        else yellow = ((t - 2000) / 200) % 2 == 1 || t > HEAT_ANIM_MS - 300;
        if (!show) return;
        Cmd(0x03F0).I(1).Run();
        for (int i = 0; i < 5; i++) {
            float x = cfgStarsX - i * cfgStarStep;
            if (cfgStarShadow) StarGlyph(x + 0.8f, cfgStarsY + 0.8f, 0, 0, 0);
            if (yellow) StarGlyph(x, cfgStarsY, 236, 182, 46);
            else        StarGlyph(x, cfgStarsY, cfgStarR, cfgStarG, cfgStarB);
        }
        return;
    }
    int tier = TierOf(g_points);
    if (tier == 0) return;
    if (cfgStarBlink && g_event == EV_AMBUSH && now - g_eventTime < 3000 && (now / 250) % 2) return;   // ambush coming
    if (g_maxSince && now - g_maxSince > cfgMaxHeatSec * 1000 - 10000 && (now / 200) % 2) return;     // last 10 s at max
    int first = 0;
    float y = cfgStarsY;
    if (cfgStarsSameRow) { Cmd w(0x01C0); w.I(0).O().Run(); first = LV(0); if (first > 6) first = 6; }
    else y += cfgStarRowGap;
    Cmd(0x03F0).I(1).Run();
    for (int i = 0; i < tier; i++) {
        float x = cfgStarsX - (first + i) * cfgStarStep;
        if (cfgStarShadow) StarGlyph(x + 0.8f, y + 0.8f, 0, 0, 0);
        StarGlyph(x, y, cfgStarR, cfgStarG, cfgStarB);
    }
}

static void DrawHud(int now, bool playing)
{
    if (!cfgShowHud) return;
    if (!g_menuOpen) {
        if (cfgEventsWhenClosed) { Cmd(0x03F0).I(1).Run(); DrawEvent(now, cfgHudX, cfgHudY, cfgHudScale); }
        return;
    }
    static const int col[6][3] = { {150,150,150}, {230,200,40}, {240,140,30}, {235,80,30}, {210,30,30}, {160,0,60} };
    const float S = cfgHudScale;
    int tier = TierOf(g_points);
    float X = cfgHudX, Y = cfgHudY, W = 124.0f * S;
    #define LY(v) (Y + (v) * S)

    Cmd(0x03F0).I(1).Run();
    Rect(X - 4 * S, Y - 4 * S, W + 8 * S, 104 * S, 0, 0, 0, 120);           // panel

    TextStyle(0.30f * S, 1.30f * S, 255, 255, 255);
    TextN(X, Y, "RETPTS", g_points);

    float bar = W * (float)g_points / (float)(cfgMaxPoints > 0 ? cfgMaxPoints : 50);
    Rect(X, LY(15), W, 7 * S, 30, 30, 30, 220);
    if (bar > 0.5f) Rect(X, LY(15), bar, 7 * S, col[tier][0], col[tier][1], col[tier][2], 255);
    for (int i = 1; i < 5; i++) Rect(X + W * i / 5.0f, LY(15), 1, 7 * S, 0, 0, 0, 255);

    TextStyle(0.24f * S, 1.05f * S, col[tier][0], col[tier][1], col[tier][2]);
    TextN(X, LY(25), "RETLVL", tier);

    TextStyle(0.22f * S, 1.0f * S, 220, 220, 220);
    if (g_points > 0) {
        int left = (DecayRef() + cfgDecaySec * 1000 - now) / 1000; if (left < 0) left = 0;
        TextN(X, LY(36), "RETDEC", left);
    } else Text(X, LY(36), "RETCLR");

    TextStyle(0.22f * S, 1.0f * S, 220, 220, 220);
    TextN(X, LY(46), "RETHNT", ActiveHunters());

    TextStyle(0.22f * S, 1.0f * S, 120, 170, 255);
    if (!cfgWantedCap || tier == 0) Text(X, LY(56), "RETWNN");
    else TextN(X, LY(56), "RETWNT", WantedCap());

    TextStyle(0.22f * S, 1.0f * S, 220, 220, 220);
    if (!playing)                 Text(X, LY(66), "RETWAS");
    else if (tier == 0)           Text(X, LY(66), "RETOFF");
    else if (g_pauseReason == 1)  Text(X, LY(66), "RETPAM");
    else if (g_pauseReason == 2)  Text(X, LY(66), "RETPAI");
    else if (g_pending)           Text(X, LY(66), "RETLOD");
    else { int s = (g_nextWave - now) / 1000; if (s < 0) s = 0; TextN(X, LY(66), "RETNXT", s); }

    TextStyle(0.22f * S, 1.0f * S, 255, 220, 120);
    Text(X, LY(78), "RETMNU");

    DrawEvent(now, X, LY(90), S);
    #undef LY
}

// ---------------------------------------------------------------------
// Menu keys: F8 toggles the panel; Left/Right change level, Up forces an ambush
// ---------------------------------------------------------------------
static bool GameFocused()
{
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

static void SetTier(int tier, int now)
{
    if (tier < 0) tier = 0;
    int pts = tier * 10; if (pts > cfgMaxPoints) pts = cfgMaxPoints;
    if (pts > g_points) g_lastGain = now; else g_lastDecay = now;
    Log("[%d] menu: points %d -> %d", now, g_points, pts);
    g_points = pts;
    SetEvent(EV_DEBUG, TierOf(g_points));
}

static void DebugKeys(int now)
{
    static bool prev[5];
    const int keys[5] = { cfgKeyMenu, cfgKeyDown, cfgKeyUp, cfgKeyAmbush, cfgKeyHeli };
    bool focus = GameFocused();
    for (int k = 0; k < 5; k++) {
        bool down = focus && (GetAsyncKeyState(keys[k]) & 0x8000);
        if (down && !prev[k]) {
            if (k == 0) g_menuOpen = !g_menuOpen;
            else if (g_menuOpen) {
                int tier = TierOf(g_points);
                if (k == 1) SetTier(tier - 1, now);
                else if (k == 2) SetTier(tier + 1, now);
                else if (k == 3) { if (tier > 0 && !g_pending) g_nextWave = now; }    // force ambush now
                else if (tier > 0 && g_heli.state == 0) { g_heli.cooldownUntil = 0; g_heliRequest = true; }   // force helicopter
            }
        }
        prev[k] = down;
    }
}

// ---------------------------------------------------------------------
// Main tick (called by Retaliation.cs every frame through opcode 1FA5)
// ---------------------------------------------------------------------
static void Tick()
{
    int now = Now();
    if (g_lastNow >= 0 && now < g_lastNow) { Log("[%d] time went backwards -> game loaded, state reset", now); ResetState(); }
    g_lastNow = now;

    uint8_t* player = *(uint8_t**)ADDR_PLAYER_PED;
    if (!player) return;

    if (!g_init) {
        ScanKills(false); ScanThefts(false);
        g_lastDecay = now; g_nextWave = now + cfgFirstWaveSec * 1000; g_init = true;
        Log("[%d] baseline taken", now);
    }

    Cmd pc(0x01F5); pc.I(0).O().Run(); int playerChar = LV(0);
    bool playing = Cmd(0x0256).I(0).Run();

    if (!playing) {
        if (!g_deadHandled) {
            g_deadHandled = true;
            if (g_lock) { g_lock = false; Log("[%d] retaliation unlocked (wasted/busted)", now); }
            g_heatAnim = 0;
            g_maxSince = 0;
            CleanupAll();
            if (cfgDeathPenalty > 0 && g_points > 0) AddPoints(-cfgDeathPenalty, EV_DEATH);
            g_nextWave = now + 30000;
        }
        ScanKills(false); ScanThefts(false);
        DrawHud(now, false);
        DrawStars(now, false);
        return;
    }
    g_deadHandled = false;

    if (cfgDebugKeys) DebugKeys(now);
    ScanKills(true);
    ScanThefts(true);
    UpdateDecay(now);

    float px, py, pz; CharPos(playerChar, &px, &py, &pz);
    UpdateHunters(now, playerChar, px, py);
    UpdateSpawner(now, playerChar, px, py, pz);
    UpdateHeli(now, playerChar, px, py, pz);
    UpdateMaxHeat(now);
    UpdateWanted();
    DrawHud(now, true);
    DrawStars(now, true);
}

static int __stdcall Opcode_Tick(void* /*thread*/)
{
    if (g_enabled) Tick();
    return 0; // OR_CONTINUE
}

// ---------------------------------------------------------------------
// CLEO hookup
// ---------------------------------------------------------------------
typedef BOOL (__stdcall* RegisterOpcode_t)(WORD, int (__stdcall*)(void*));
typedef int  (__stdcall* GetGameVersion_t)();

static bool TryRegister()
{
    HMODULE cleo = GetModuleHandleA("CLEO.asi");
    if (!cleo) return false;
    RegisterOpcode_t reg = (RegisterOpcode_t)GetProcAddress(cleo, "_CLEO_RegisterOpcode@8");
    GetGameVersion_t ver = (GetGameVersion_t)GetProcAddress(cleo, "_CLEO_GetGameVersion@0");
    if (!reg) { Log("CLEO_RegisterOpcode not found"); return true; }
    int gv = ver ? ver() : -1;
    if (gv != 0) { Log("Unsupported game version %d (need 1.0 US). Mod disabled.", gv); g_enabled = false; }
    BOOL ok = reg(CUSTOM_OPCODE, Opcode_Tick);
    Log("Opcode %04X registered: %s (game version %d)", CUSTOM_OPCODE, ok ? "OK" : "FAILED", gv);
    return true;
}

static DWORD WINAPI WaitCleoThread(LPVOID)
{
    for (int i = 0; i < 600; i++) { if (TryRegister()) return 0; Sleep(100); }
    Log("CLEO.asi not found - mod inactive");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        GetModuleFileNameA(inst, g_dir, MAX_PATH);
        char* slash = strrchr(g_dir, '\\'); if (slash) slash[1] = 0;
        sprintf(g_iniPath, "%sRetaliation.ini", g_dir);
        sprintf(g_logPath, "%sRetaliation.log", g_dir);
        DeleteFileA(g_logPath);
        Log("Retaliation v1.05 loaded from %s", g_dir);
        LoadConfig();
        g_rng = GetTickCount() | 1;
        if (!TryRegister()) CreateThread(0, 0, WaitCleoThread, 0, 0, 0);
    }
    return TRUE;
}
