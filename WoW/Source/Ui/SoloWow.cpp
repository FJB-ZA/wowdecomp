#include "SoloWow.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Types.h"

// solo-wow addition, not part of the original client. See SoloWow.h.

static constexpr uint32_t SOLOWOW_MAX_PROGRESSION_STEP = 18;
static constexpr uint32_t SOLOWOW_MAX_SPEC_SLOTS = 4;

// What the server last sent, and for which character
struct SoloWowStatus {
    WOWGUID player;
    bool hasProgression;
    uint32_t progression;
    bool hasXpRate;
    uint32_t xpRateHundredths;
    uint32_t xpMaxRateHundredths;
    bool xpPaused;
    bool hasSpecSlots;
    uint32_t specActive;
    uint32_t specOwned;
    uint32_t specNextPrice;
    uint32_t specPoints[SOLOWOW_MAX_SPEC_SLOTS][3];
};

static SoloWowStatus s_status;

// The status of the character in the world, or nullptr when it belongs to an earlier character
static const SoloWowStatus* CurrentStatus() {
    WOWGUID player = ClntObjMgrGetActivePlayer();
    return player && s_status.player == player ? &s_status : nullptr;
}

// The status to update for the character in the world: an earlier character's is cleared first
static SoloWowStatus& StatusToUpdate() {
    WOWGUID player = ClntObjMgrGetActivePlayer();

    if (s_status.player != player) {
        memset(&s_status, 0, sizeof(s_status));
        s_status.player = player;
    }

    return s_status;
}

// Reads an unsigned decimal number at `text`. Returns the character after it, or nullptr when there is none.
static const char* ReadNumber(const char* text, uint32_t& value) {
    if (*text < '0' || *text > '9') {
        return nullptr;
    }

    uint64_t number = 0;

    for (; *text >= '0' && *text <= '9'; text++) {
        number = number * 10 + static_cast<uint32_t>(*text - '0');

        if (number > 0xFFFFFFFFull) {
            return nullptr;
        }
    }

    value = static_cast<uint32_t>(number);
    return text;
}

// Reads " <number>" at `text`
static const char* ReadField(const char* text, uint32_t& value) {
    return text && *text == ' ' ? ReadNumber(text + 1, value) : nullptr;
}

// "P1 <step>"
static bool ParseProgression(const char* message) {
    uint32_t step;
    const char* end = ReadField(message + 2, step);

    if (!end || *end || step > SOLOWOW_MAX_PROGRESSION_STEP) {
        return false;
    }

    SoloWowStatus& status = StatusToUpdate();
    status.hasProgression = true;
    status.progression = step;
    FrameScript_SignalEvent(SCRIPT_EVENT_SOLOWOW_PROGRESSION_UPDATE, "%d", step);
    return true;
}

// "X1 <rate x 100> <maximum rate x 100> <paused 0 or 1>"
static bool ParseXpRate(const char* message) {
    uint32_t rate, maxRate, paused;
    const char* end = ReadField(ReadField(ReadField(message + 2, rate), maxRate), paused);

    if (!end || *end || paused > 1) {
        return false;
    }

    SoloWowStatus& status = StatusToUpdate();
    status.hasXpRate = true;
    status.xpRateHundredths = rate;
    status.xpMaxRateHundredths = maxRate;
    status.xpPaused = paused == 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_SOLOWOW_XP_RATE_UPDATE, nullptr);
    return true;
}

// "S1 <active> <owned> <nextPriceCopper> <p1> <p2> <p3> <p4>", where pN is "a/b/c", or "-" for a slot not bought
static bool ParseSpecSlots(const char* message) {
    uint32_t active, owned, nextPrice;
    const char* text = ReadField(ReadField(ReadField(message + 2, active), owned), nextPrice);

    if (!text || owned < 1 || owned > SOLOWOW_MAX_SPEC_SLOTS || active < 1 || active > owned) {
        return false;
    }

    uint32_t points[SOLOWOW_MAX_SPEC_SLOTS][3] = {};

    for (uint32_t slot = 0; slot < SOLOWOW_MAX_SPEC_SLOTS; slot++) {
        if (*text++ != ' ') {
            return false;
        }

        if (slot >= owned) {
            if (*text++ != '-') {
                return false;
            }

            continue;
        }

        for (uint32_t tree = 0; tree < 3; tree++) {
            if (tree > 0 && *text++ != '/') {
                return false;
            }

            text = ReadNumber(text, points[slot][tree]);

            if (!text) {
                return false;
            }
        }
    }

    if (*text) {
        return false;
    }

    SoloWowStatus& status = StatusToUpdate();
    status.hasSpecSlots = true;
    status.specActive = active;
    status.specOwned = owned;
    status.specNextPrice = nextPrice;
    memcpy(status.specPoints, points, sizeof(points));
    FrameScript_SignalEvent(SCRIPT_EVENT_SOLOWOW_SPEC_SLOTS_UPDATE, nullptr);
    return true;
}

bool SoloWow::HandleAddonMessage(const char* prefix, const char* message) {
    if (!strcmp(prefix, "SoloWow")) {
        if (!strncmp(message, "P1", 2)) {
            return ParseProgression(message);
        }

        if (!strncmp(message, "X1", 2)) {
            return ParseXpRate(message);
        }

        return false;
    }

    if (!strcmp(prefix, "SoloWowSlots") && !strncmp(message, "S1", 2)) {
        return ParseSpecSlots(message);
    }

    return false;
}

static int32_t Script_GetSoloWowProgression(lua_State* L) {
    const SoloWowStatus* status = CurrentStatus();

    if (!status || !status->hasProgression) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, status->progression);
    return 1;
}

static int32_t Script_GetSoloWowXPRate(lua_State* L) {
    const SoloWowStatus* status = CurrentStatus();

    if (!status || !status->hasXpRate) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, status->xpRateHundredths / 100.0);
    lua_pushnumber(L, status->xpMaxRateHundredths / 100.0);

    if (status->xpPaused) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 3;
}

static int32_t Script_GetNumSpecSlots(lua_State* L) {
    const SoloWowStatus* status = CurrentStatus();

    if (!status || !status->hasSpecSlots) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, status->specOwned);
    lua_pushnumber(L, status->specActive);
    lua_pushnumber(L, status->specNextPrice);
    return 3;
}

static int32_t Script_GetSpecSlotTalents(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetSpecSlotTalents(slot)");
        return 0;
    }

    const SoloWowStatus* status = CurrentStatus();
    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (!status || !status->hasSpecSlots || slot < 1 || slot > status->specOwned) {
        lua_pushnil(L);
        return 1;
    }

    for (uint32_t tree = 0; tree < 3; tree++) {
        lua_pushnumber(L, status->specPoints[slot - 1][tree]);
    }

    return 3;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "GetSoloWowProgression", &Script_GetSoloWowProgression },
    { "GetSoloWowXPRate", &Script_GetSoloWowXPRate },
    { "GetNumSpecSlots", &Script_GetNumSpecSlots },
    { "GetSpecSlotTalents", &Script_GetSpecSlotTalents },
};

void SoloWow::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

void SoloWow::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
