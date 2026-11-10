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
#define VEH_FLAGS_2     0x42A               // bit1 = bHasBeenOwnedByPlayer

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
    int squad, maxActive, intervalSec, accuracy, health, armour, carChance, groups;
    float footMin, footMax, cruise;
};

static TierCfg g_tier[6];
static int   cfgMaxPoints = 50, cfgKill = 5, cfgTheft = 2, cfgDecayAmount = 10, cfgDecaySec = 300;
static int   cfgDeathPenalty = 10, cfgCountHunterKills = 0, cfgPauseOnMission = 1;
static int   cfgShowHud = 1, cfgShowBlips = 1, cfgDebugKeys = 1, cfgFirstWaveSec = 20;
static float cfgHudX = 12.0f, cfgHudY = 150.0f, cfgDespawnDist = 220.0f, cfgDismountDist = 30.0f;

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
    cfgKill            = IniInt("Points", "KillPoints", 5);
    cfgTheft           = IniInt("Points", "TheftPoints", 2);
    cfgDecayAmount     = IniInt("Points", "DecayAmount", 10);
    cfgDecaySec        = IniInt("Points", "DecayIntervalSeconds", 300);
    cfgDeathPenalty    = IniInt("Points", "DeathPenalty", 10);
    cfgCountHunterKills= IniInt("Points", "CountHunterKills", 0);

    cfgPauseOnMission  = IniInt("General", "PauseSpawnsOnMission", 1);
    cfgShowBlips       = IniInt("General", "ShowBlips", 1);
    cfgFirstWaveSec    = IniInt("General", "FirstWaveDelaySeconds", 20);
    cfgDespawnDist     = IniFloat("General", "DespawnDistance", 220.0f);
    cfgDismountDist    = IniFloat("General", "DismountDistance", 30.0f);
    cfgDebugKeys       = IniInt("General", "DebugKeys", 1);

    cfgShowHud         = IniInt("HUD", "ShowHud", 1);
    cfgHudX            = IniFloat("HUD", "X", 12.0f);
    cfgHudY            = IniFloat("HUD", "Y", 150.0f);

    // defaults per tier -----------------------------------------------
    const char* mafia   = "111,112,124,125,126,127";
    const char* t4m     = "158,159,160,161,162,200,102,103,104,108,109,110";
    const char* t5m     = "158,159,160,161,162,200,121,122,123,117,118,173,174,175,114,115,116";
    struct Def { const char *models, *weapons, *cars; int squad, maxA, interval, acc, hp, arm, carCh, groups; float fmin, fmax, cruise; };
    static const Def defs[6] = {
        { "", "", "", 0,0,0,0,0,0,0,0, 0,0,0 },
        { mafia, "22,23",          "405,445,421,551", 2,  3, 120, 30, 100,   0,  0, 1, 40, 60, 30 },
        { mafia, "22,24,25",       "405,445,421,551", 3,  6,  90, 45, 100,   0, 40, 1, 38, 58, 35 },
        { mafia, "24,28,32,29,26", "405,445,421,551", 4,  8,  70, 55, 100,  25, 50, 2, 35, 55, 38 },
        { t4m,   "30,29,26,25",    "543,478,422,489,412,567,566", 5, 10, 50, 65, 120, 50, 60, 2, 30, 50, 42 },
        { t5m,   "31,30,27",       "495,489,579,470,400,567",     8, 14, 35, 80, 150, 100, 70, 3, 28, 45, 48 },
    };
    for (int t = 1; t <= 5; t++) {
        char sec[16]; sprintf(sec, "Tier%d", t);
        TierCfg& c = g_tier[t];
        IniList(sec, "PedModels", defs[t].models, c.models, 48, &c.nModels);
        IniList(sec, "Weapons",   defs[t].weapons, c.weapons, 16, &c.nWeapons);
        IniList(sec, "Cars",      defs[t].cars, c.cars, 16, &c.nCars);
        c.squad       = IniInt(sec, "PedsPerAmbush",  defs[t].squad);
        c.maxActive   = IniInt(sec, "MaxActive",      defs[t].maxA);
        c.intervalSec = IniInt(sec, "IntervalSeconds",defs[t].interval);
        c.accuracy    = IniInt(sec, "Accuracy",       defs[t].acc);
        c.health      = IniInt(sec, "Health",         defs[t].hp);
        c.armour      = IniInt(sec, "Armour",         defs[t].arm);
        c.carChance   = IniInt(sec, "CarChance",      defs[t].carCh);
        c.groups      = IniInt(sec, "FlankGroups",    defs[t].groups);
        c.footMin     = IniFloat(sec, "SpawnMinDist", defs[t].fmin);
        c.footMax     = IniFloat(sec, "SpawnMaxDist", defs[t].fmax);
        c.cruise      = IniFloat(sec, "CarSpeed",     defs[t].cruise);
        if (c.groups < 1) c.groups = 1;
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

// ---------------------------------------------------------------------
// State
// ---------------------------------------------------------------------
enum { EV_NONE, EV_KILL, EV_THEFT, EV_DECAY, EV_AMBUSH, EV_DEATH, EV_DEBUG };

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

struct Hunter { bool used; int ped, blip, squad, state, stateTime; };
struct Squad  { bool used; int car, spawnTime, slowSince; bool dismount; };
#define MAX_HUNTERS 40
#define MAX_SQUADS  10
static Hunter g_h[MAX_HUNTERS];
static Squad  g_s[MAX_SQUADS];

static int g_exHunters[128]; static int g_exPos = 0;

// pending wave (waiting for models)
struct PendingPed { int model, weapon, group; bool inCar; };
static bool g_pending = false;
static int  g_pendTier = 0, g_pendStart = 0, g_pendCar = -1, g_pendCount = 0;
static PendingPed g_pend[24];
static int  g_reqModels[48]; static int g_nReq = 0;

// pool tracking
#define MAX_SLOTS 8192
static uint8_t g_pedId[MAX_SLOTS], g_pedCounted[MAX_SLOTS];
static uint8_t g_vehId[MAX_SLOTS], g_vehOwned[MAX_SLOTS];

static int TierOf(int pts) { int t = pts / 10; return t < 0 ? 0 : (t > 5 ? 5 : t); }

static void SetEvent(int ev, int val) { g_event = ev; g_eventValue = val; g_eventTime = Now(); }

static void AddPoints(int delta, int ev)
{
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

static int ActiveHunters() { int n = 0; for (int i = 0; i < MAX_HUNTERS; i++) if (g_h[i].used) n++; return n; }

static void ReleaseHunter(Hunter& h, bool exists)
{
    if (h.blip) Cmd(0x0164).I(h.blip).Run();
    if (exists) Cmd(0x01C2).I(h.ped).Run();
    g_exHunters[g_exPos] = h.ped; g_exPos = (g_exPos + 1) % 128;
    h.used = false; h.blip = 0;
}

static void ReleaseSquad(Squad& s)
{
    if (s.car && CarExists(s.car)) Cmd(0x01C3).I(s.car).Run();
    s.used = false; s.car = 0;
}

static void CancelPending()
{
    for (int i = 0; i < g_nReq; i++) Cmd(0x0249).I(g_reqModels[i]).Run();
    g_nReq = 0; g_pending = false;
}

static void CleanupAll()
{
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
    g_pending = false; g_nReq = 0;
    g_points = 0; g_lastGain = g_lastDecay = 0; g_prevTier = 0;
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
        AddPoints(cfgKill, EV_KILL);
    }
}

static void ScanThefts(bool award)
{
    CPool* pool = *(CPool**)ADDR_VEH_POOL;
    uint8_t* player = *(uint8_t**)ADDR_PLAYER_PED;
    if (!pool || !player) return;
    float px, py, pz; EntityPos(player, &px, &py, &pz);
    int n = pool->size; if (n > MAX_SLOTS) n = MAX_SLOTS;
    for (int i = 0; i < n; i++) {
        uint8_t b = pool->byteMap[i];
        if (b & 0x80) { g_vehId[i] = 0xFF; continue; }
        uint8_t* veh = pool->objects + i * VEH_SIZE;
        uint8_t owned = (veh[VEH_FLAGS_2] >> 1) & 1;
        if (g_vehId[i] != b) { g_vehId[i] = b; g_vehOwned[i] = owned; continue; }
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
        if (!any) { ReleaseSquad(q); continue; }
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
        if (CharDead(h.ped))    { ReleaseHunter(h, true);  continue; }
        float x, y, z; CharPos(h.ped, &x, &y, &z);
        if (Dist2D(x, y, px, py) > cfgDespawnDist) { ReleaseHunter(h, true); continue; }

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

static void PrepareWave(int tier)
{
    TierCfg& c = g_tier[tier];
    int room = c.maxActive - ActiveHunters();
    int count = c.squad < room ? c.squad : room;
    if (count > 24) count = 24;
    if (count < 1 || c.nModels == 0 || c.nWeapons == 0) return;

    g_nReq = 0; g_pendCount = count; g_pendTier = tier; g_pendStart = Now(); g_pendCar = -1;
    int inCar = 0;
    if (c.nCars > 0 && RandInt(1, 100) <= c.carChance) {
        g_pendCar = c.cars[RandInt(0, c.nCars - 1)];
        AddReq(g_pendCar);
        inCar = count < 4 ? count : 4;
    }
    int footCount = count - inCar;
    int groups = c.groups; if (groups > footCount) groups = footCount > 0 ? footCount : 1;
    for (int i = 0; i < count; i++) {
        PendingPed& p = g_pend[i];
        p.model  = c.models[RandInt(0, c.nModels - 1)];
        p.weapon = c.weapons[RandInt(0, c.nWeapons - 1)];
        p.inCar  = i < inCar;
        p.group  = p.inCar ? -1 : (i - inCar) % groups;
        AddReq(p.model);
        int wm = WeaponModel(p.weapon);
        if (wm > 0) AddReq(wm);
    }
    g_pending = true;
}

static bool OnScreen(float x, float y, float z) { return Cmd(0x00C2).F(x).F(y).F(z).F(2.5f).Run(); }

static bool FindFootSpawn(float px, float py, float pz, float baseAng, float minR, float maxR, float* ox, float* oy, float* oz)
{
    for (int t = 0; t < 10; t++) {
        float ang = baseAng + RandF(-0.6f, 0.6f) + (t > 4 ? RandF(-3.14f, 3.14f) : 0.0f);
        float r = RandF(minR, maxR);
        Cmd c(0x02C0); c.F(px + cosf(ang) * r).F(py + sinf(ang) * r).F(pz).O().O().O().Run();
        float nx = LVF(0), ny = LVF(1), nz = LVF(2);
        float d = Dist2D(nx, ny, px, py);
        if (d < minR * 0.6f || d > maxR * 1.6f || fabsf(nz - pz) > 15.0f) continue;
        if (OnScreen(nx, ny, nz + 1.0f)) continue;
        *ox = nx; *oy = ny; *oz = nz; return true;
    }
    return false;
}

static bool FindCarSpawn(float px, float py, float pz, float* ox, float* oy, float* oz)
{
    for (int t = 0; t < 10; t++) {
        float ang = RandF(-3.14159f, 3.14159f), r = RandF(100.0f, 150.0f);
        Cmd c(0x02C1); c.F(px + cosf(ang) * r).F(py + sinf(ang) * r).F(pz).O().O().O().Run();
        float nx = LVF(0), ny = LVF(1), nz = LVF(2);
        float d = Dist2D(nx, ny, px, py);
        if (d < 70.0f || d > 220.0f || fabsf(nz - pz) > 25.0f) continue;
        if (OnScreen(nx, ny, nz + 1.0f)) continue;
        *ox = nx; *oy = ny; *oz = nz; return true;
    }
    return false;
}

static int FreeHunterSlot() { for (int i = 0; i < MAX_HUNTERS; i++) if (!g_h[i].used) return i; return -1; }
static int FreeSquadSlot()  { for (int i = 0; i < MAX_SQUADS; i++)  if (!g_s[i].used) return i; return -1; }

static void SetupHunter(int ped, const PendingPed& p, const TierCfg& c, int squad, int state, int playerChar)
{
    int slot = FreeHunterSlot();
    Cmd(0x01B2).I(ped).I(p.weapon).I(9999).Run();
    Cmd(0x01B9).I(ped).I(p.weapon).Run();
    Cmd(0x02E2).I(ped).I(c.accuracy).Run();
    Cmd(0x0223).I(ped).I(c.health).Run();
    if (c.armour > 0) Cmd(0x035F).I(ped).I(c.armour).Run();
    int blip = 0;
    if (cfgShowBlips) {
        Cmd b(0x0187); b.I(ped).O().Run(); blip = LV(0);
        Cmd(0x0165).I(blip).I(0).Run();       // red
    }
    if (state == 2) AttackPlayer(ped, playerChar);
    if (slot < 0) { if (blip) Cmd(0x0164).I(blip).Run(); Cmd(0x01C2).I(ped).Run(); return; }
    Hunter& h = g_h[slot];
    h.used = true; h.ped = ped; h.blip = blip; h.squad = squad; h.state = state; h.stateTime = Now();
}

static bool SpawnWave(int playerChar, float px, float py, float pz)
{
    TierCfg& c = g_tier[g_pendTier];
    int spawned = 0;

    // ---- car squad
    if (g_pendCar >= 0) {
        float x, y, z; int sq = FreeSquadSlot();
        if (sq >= 0 && FindCarSpawn(px, py, pz, &x, &y, &z)) {
            Cmd cc(0x00A5); cc.I(g_pendCar).F(x).F(y).F(z).O().Run();
            int car = LV(0);
            float heading = atan2f(-(px - x), (py - y)) * 57.29578f;
            Cmd(0x0175).I(car).F(heading).Run();
            Cmd mp(0x01EA); mp.I(car).O().Run(); int seats = LV(0);
            Squad& q = g_s[sq];
            q.used = true; q.car = car; q.spawnTime = Now(); q.slowSince = 0; q.dismount = false;
            int seat = 0; bool driver = false;
            for (int i = 0; i < g_pendCount; i++) {
                PendingPed& p = g_pend[i];
                if (!p.inCar) continue;
                int ped;
                if (!driver) { Cmd d(0x0129); d.I(car).I(PEDTYPE_MISSION1).I(p.model).O().Run(); ped = LV(0); driver = true; }
                else if (seat < seats) { Cmd d(0x01C8); d.I(car).I(PEDTYPE_MISSION1).I(p.model).I(seat).O().Run(); ped = LV(0); seat++; }
                else { p.inCar = false; p.group = 0; continue; }     // no seat: goes on foot
                SetupHunter(ped, p, c, sq, 0, playerChar);
                spawned++;
            }
            Cmd(0x00AE).I(car).I(2).Run();                  // avoid cars
            Cmd(0x00AD).I(car).F(c.cruise).Run();
            Cmd(0x00AF).I(car).I(2).Run();                  // MISSION_RAMPLAYER_FARAWAY
        } else {
            for (int i = 0; i < g_pendCount; i++) if (g_pend[i].inCar) { g_pend[i].inCar = false; g_pend[i].group = i % c.groups; }
        }
    }

    // ---- foot groups (flanking from different directions)
    float baseAng = RandF(-3.14159f, 3.14159f);
    for (int g = 0; g < c.groups; g++) {
        bool need = false;
        for (int i = 0; i < g_pendCount; i++) if (!g_pend[i].inCar && g_pend[i].group == g) { need = true; break; }
        if (!need) continue;
        float ang = baseAng + g * (6.28318f / c.groups);
        float x, y, z;
        if (!FindFootSpawn(px, py, pz, ang, c.footMin, c.footMax, &x, &y, &z)) continue;
        int k = 0;
        for (int i = 0; i < g_pendCount; i++) {
            PendingPed& p = g_pend[i];
            if (p.inCar || p.group != g) continue;
            float ox = (k % 3) * 1.4f - 1.4f, oy = (k / 3) * 1.4f;
            Cmd cc(0x009A); cc.I(PEDTYPE_MISSION1).I(p.model).F(x + ox).F(y + oy).F(z).O().Run();
            SetupHunter(LV(0), p, c, -1, 2, playerChar);
            k++; spawned++;
        }
    }
    return spawned > 0;
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

    if (g_pending) {
        if (AllLoaded()) {
            bool ok = SpawnWave(playerChar, px, py, pz);
            CancelPending();                          // releases model requests
            if (ok) {
                SetEvent(EV_AMBUSH, tier);
                Log("[%d] ambush spawned (tier %d)", now, tier);
                int iv = g_tier[tier].intervalSec * 1000;
                g_nextWave = now + RandInt(iv * 8 / 10, iv * 12 / 10);
            } else {
                g_nextWave = now + 4000;              // no valid spot, retry soon
            }
        } else if (now - g_pendStart > 15000) {
            CancelPending(); g_nextWave = now + 5000;
        }
        return;
    }
    if (now >= g_nextWave) {
        if (ActiveHunters() >= g_tier[tier].maxActive) { g_nextWave = now + 5000; return; }
        PrepareWave(tier);
        if (!g_pending) g_nextWave = now + 5000;
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

static void DrawHud(int now, bool playing)
{
    if (!cfgShowHud) return;
    static const int col[6][3] = { {150,150,150}, {230,200,40}, {240,140,30}, {235,80,30}, {210,30,30}, {160,0,60} };
    int tier = TierOf(g_points);
    float X = cfgHudX, Y = cfgHudY, W = 124.0f;

    Cmd(0x03F0).I(1).Run();
    Rect(X - 4, Y - 4, W + 8, 82, 0, 0, 0, 120);                 // panel

    TextStyle(0.30f, 1.30f, 255, 255, 255);
    TextN(X, Y, "RETPTS", g_points);

    float bar = W * (float)g_points / (float)(cfgMaxPoints > 0 ? cfgMaxPoints : 50);
    Rect(X, Y + 15, W, 7, 30, 30, 30, 220);
    if (bar > 0.5f) Rect(X, Y + 15, bar, 7, col[tier][0], col[tier][1], col[tier][2], 255);
    for (int i = 1; i < 5; i++) Rect(X + W * i / 5.0f, Y + 15, 1, 7, 0, 0, 0, 255);

    TextStyle(0.24f, 1.05f, col[tier][0], col[tier][1], col[tier][2]);
    TextN(X, Y + 25, "RETLVL", tier);

    TextStyle(0.22f, 1.0f, 220, 220, 220);
    if (g_points > 0) {
        int left = (DecayRef() + cfgDecaySec * 1000 - now) / 1000; if (left < 0) left = 0;
        TextN(X, Y + 36, "RETDEC", left);
    } else Text(X, Y + 36, "RETCLR");

    TextStyle(0.22f, 1.0f, 220, 220, 220);
    TextN(X, Y + 46, "RETHNT", ActiveHunters());

    TextStyle(0.22f, 1.0f, 220, 220, 220);
    if (!playing)                 Text(X, Y + 56, "RETWAS");
    else if (tier == 0)           Text(X, Y + 56, "RETOFF");
    else if (g_pauseReason == 1)  Text(X, Y + 56, "RETPAM");
    else if (g_pauseReason == 2)  Text(X, Y + 56, "RETPAI");
    else if (g_pending)           Text(X, Y + 56, "RETLOD");
    else { int s = (g_nextWave - now) / 1000; if (s < 0) s = 0; TextN(X, Y + 56, "RETNXT", s); }

    if (g_event != EV_NONE && now - g_eventTime < 3500) {
        const char* key = 0; int r = 255, g = 255, b = 255; bool num = false;
        switch (g_event) {
            case EV_KILL:   key = "RETEVK"; r = 255; g = 90;  b = 90;  num = true; break;
            case EV_THEFT:  key = "RETEVC"; r = 255; g = 170; b = 60;  num = true; break;
            case EV_DECAY:  key = "RETEVD"; r = 90;  g = 220; b = 90;  num = true; break;
            case EV_AMBUSH: key = "RETEVA"; r = 255; g = 40;  b = 40;  break;
            case EV_DEATH:  key = "RETEVX"; r = 180; g = 180; b = 255; num = true; break;
            case EV_DEBUG:  key = "RETEVG"; r = 200; g = 200; b = 255; num = true; break;
        }
        if (key) {
            TextStyle(0.26f, 1.15f, r, g, b);
            int v = g_eventValue < 0 ? -g_eventValue : g_eventValue;
            if (num) TextN(X, Y + 68, key, v); else Text(X, Y + 68, key);
        }
    }
}

// ---------------------------------------------------------------------
// Debug keys (NumPad + / NumPad - / NumPad *)
// ---------------------------------------------------------------------
static bool GameFocused()
{
    DWORD pid = 0; GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

static void DebugKeys(int now)
{
    static bool prev[3];
    const int keys[3] = { VK_ADD, VK_SUBTRACT, VK_MULTIPLY };
    bool focus = GameFocused();
    for (int k = 0; k < 3; k++) {
        bool down = focus && (GetAsyncKeyState(keys[k]) & 0x8000);
        if (down && !prev[k]) {
            if (k == 0) AddPoints(10, EV_DEBUG);
            else if (k == 1) { AddPoints(-10, EV_DEBUG); g_lastDecay = now; }
            else if (TierOf(g_points) > 0 && !g_pending) g_nextWave = now;   // force ambush now
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
            CleanupAll();
            if (cfgDeathPenalty > 0 && g_points > 0) AddPoints(-cfgDeathPenalty, EV_DEATH);
            g_nextWave = now + 30000;
        }
        ScanKills(false); ScanThefts(false);
        DrawHud(now, false);
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
    DrawHud(now, true);
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
        Log("Retaliation v1.0 loaded from %s", g_dir);
        LoadConfig();
        g_rng = GetTickCount() | 1;
        if (!TryRegister()) CreateThread(0, 0, WaitCleoThread, 0, 0, 0);
    }
    return TRUE;
}
