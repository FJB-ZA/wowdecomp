#include "SoloWowNamePlates.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/NamePlateFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"

// solo-wow addition, not part of the original client. See SoloWowNamePlates.h.

static constexpr uint32_t SOLOWOW_MAX_NAMEPLATE_UNITS = 100;

// The unit holding each token; nameplate1 is index 0
static WOWGUID s_namePlateUnits[SOLOWOW_MAX_NAMEPLATE_UNITS];

static void FormatToken(char* buffer, uint32_t size, uint32_t index) {
    SStrPrintf(buffer, size, "nameplate%d", index + 1);
}

static void PushFrame(lua_State* L, CGNamePlateFrame* frame) {
    if (!frame->m_luaRegistered) {
        frame->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, frame->m_luaObjectRef);
}

// frame.namePlateUnitToken = token (nil when token is nullptr)
static void SetTokenField(CGNamePlateFrame* frame, const char* token) {
    lua_State* L = FrameScript_GetContext();

    if (!L || !frame) {
        return;
    }

    PushFrame(L, frame);
    lua_pushstring(L, "namePlateUnitToken");

    if (token) {
        lua_pushstring(L, token);
    } else {
        lua_pushnil(L);
    }

    lua_rawset(L, -3);
    lua_pop(L, 1);
}

int32_t SoloWowNamePlates::GetIndex(const WOWGUID& guid) {
    if (!guid) {
        return -1;
    }

    for (uint32_t i = 0; i < SOLOWOW_MAX_NAMEPLATE_UNITS; i++) {
        if (s_namePlateUnits[i] == guid) {
            return static_cast<int32_t>(i);
        }
    }

    return -1;
}

WOWGUID SoloWowNamePlates::GetUnit(uint32_t index) {
    return index < SOLOWOW_MAX_NAMEPLATE_UNITS ? s_namePlateUnits[index] : WOWGUID();
}

void SoloWowNamePlates::OnAdded(CGUnit_C* unit) {
    WOWGUID guid = unit->m_obj->m_guid;

    if (GetIndex(guid) >= 0) {
        return;
    }

    uint32_t index = 0;

    while (index < SOLOWOW_MAX_NAMEPLATE_UNITS && s_namePlateUnits[index]) {
        index++;
    }

    // More nameplates than tokens: this one stays without a token
    if (index == SOLOWOW_MAX_NAMEPLATE_UNITS) {
        return;
    }

    s_namePlateUnits[index] = guid;

    if (!CGGameUI::m_initialized) {
        return;
    }

    char token[32];
    FormatToken(token, sizeof(token), index);
    SetTokenField(unit->m_namePlateFrame, token);
    FrameScript_SignalEvent(SCRIPT_EVENT_NAME_PLATE_UNIT_ADDED, "%s", token);
}

void SoloWowNamePlates::OnRemoved(CGUnit_C* unit) {
    int32_t index = GetIndex(unit->m_obj->m_guid);

    if (index < 0) {
        return;
    }

    if (CGGameUI::m_initialized) {
        char token[32];
        FormatToken(token, sizeof(token), static_cast<uint32_t>(index));
        FrameScript_SignalEvent(SCRIPT_EVENT_NAME_PLATE_UNIT_REMOVED, "%s", token);
        SetTokenField(unit->m_namePlateFrame, nullptr);
    }

    s_namePlateUnits[index] = WOWGUID();
}

void SoloWowNamePlates::Clear() {
    memset(s_namePlateUnits, 0, sizeof(s_namePlateUnits));
}

static int32_t Script_GetNamePlateForUnit(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetNamePlateForUnit(unit)");
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tostring(L, 1));

    if (!unit || !unit->m_namePlateFrame) {
        lua_pushnil(L);
        return 1;
    }

    PushFrame(L, unit->m_namePlateFrame);
    return 1;
}

static int32_t Script_GetNamePlates(lua_State* L) {
    lua_newtable(L);
    int32_t count = 0;

    for (uint32_t i = 0; i < SOLOWOW_MAX_NAMEPLATE_UNITS; i++) {
        if (!s_namePlateUnits[i]) {
            continue;
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_namePlateUnits[i], TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit && unit->m_namePlateFrame) {
            PushFrame(L, unit->m_namePlateFrame);
            lua_rawseti(L, -2, ++count);
        }
    }

    return 1;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNamePlateForUnit", &Script_GetNamePlateForUnit },
    { "GetNamePlates", &Script_GetNamePlates },
};

void SoloWowNamePlates::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

void SoloWowNamePlates::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
