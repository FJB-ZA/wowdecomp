#include "SoloWowTracking.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/SpellBookFrame.h"

// solo-wow addition, not part of the original client. See SoloWowTracking.h.

static constexpr uint32_t NUM_TRACKING_TYPES = sizeof(CGMinimapFrame::s_trackingTypes) /
    sizeof(CGMinimapFrame::s_trackingTypes[0]);

// Bit i: CGMinimapFrame::s_trackingTypes[i] is on
static uint32_t s_typesOn;

static int32_t IndexOf(const TrackingTypeInfo* info) {
    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        if (&CGMinimapFrame::s_trackingTypes[i] == info) {
            return static_cast<int32_t>(i);
        }
    }

    return -1;
}

void SoloWowTracking::Reset() {
    s_typesOn = 0;
}

bool SoloWowTracking::IsOn(const TrackingTypeInfo* info) {
    int32_t index = IndexOf(info);
    return index >= 0 && (s_typesOn & (1u << index));
}

void SoloWowTracking::Set(const TrackingTypeInfo* info, bool on) {
    int32_t index = IndexOf(info);

    if (index < 0) {
        return;
    }

    if (on) {
        s_typesOn |= 1u << index;
    } else {
        s_typesOn &= ~(1u << index);
    }
}

void SoloWowTracking::Clear() {
    s_typesOn = 0;
}

const TrackingTypeInfo* SoloWowTracking::ShownType(const TrackingTypeInfo* info) {
    if (info && SoloWowTracking::IsOn(info)) {
        return info;
    }

    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        if (s_typesOn & (1u << i)) {
            return &CGMinimapFrame::s_trackingTypes[i];
        }
    }

    return nullptr;
}

uint32_t SoloWowTracking::NpcFlags() {
    uint32_t flags = 0;

    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        if ((s_typesOn & (1u << i)) && CGMinimapFrame::s_trackingTypes[i].m_type == 1) {
            flags |= CGMinimapFrame::s_trackingTypes[i].m_npcFlags;
        }
    }

    return flags;
}

bool SoloWowTracking::TracksGameObjectType(uint32_t typeId) {
    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        const TrackingTypeInfo& info = CGMinimapFrame::s_trackingTypes[i];

        if ((s_typesOn & (1u << i)) && info.m_type == 2 && info.m_npcFlags == typeId) {
            return true;
        }
    }

    return false;
}

bool SoloWowTracking::TracksTrivialQuests() {
    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        if ((s_typesOn & (1u << i)) && CGMinimapFrame::s_trackingTypes[i].m_type == 3) {
            return true;
        }
    }

    return false;
}

void SoloWowTracking::FormatState(char* buffer, uint32_t size) {
    if (!size) {
        return;
    }

    buffer[0] = '\0';

    for (uint32_t i = 0; i < NUM_TRACKING_TYPES; i++) {
        if (!(s_typesOn & (1u << i))) {
            continue;
        }

        if (buffer[0]) {
            SStrPack(buffer, " ", size);
        }

        SStrPack(buffer, CGMinimapFrame::s_trackingTypes[i].m_name, size);
    }
}

static CGUnit_C* ActivePlayer() {
    return static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__,
        __LINE__));
}

bool SoloWowTracking::HasSpell(int32_t spellId) {
    CGUnit_C* player = ActivePlayer();

    if (!player || !spellId) {
        return false;
    }

    for (uint32_t i = 0; i < player->GetAuraCount(); i++) {
        if (player->GetAuraSpellId(i) == spellId) {
            return true;
        }
    }

    return false;
}

int32_t SoloWowTracking::AnySpellOnExcept(int32_t except) {
    for (uint32_t i = 0; i < s_trackingSpells.Count(); i++) {
        int32_t spellId = s_trackingSpells.m_data[i];

        if (spellId != except && SoloWowTracking::HasSpell(spellId)) {
            return spellId;
        }
    }

    return 0;
}

void SoloWowTracking::CancelSpells() {
    for (uint32_t i = 0; i < s_trackingSpells.Count(); i++) {
        SpellRec spell;

        if (SoloWowTracking::HasSpell(s_trackingSpells.m_data[i]) && g_spellDB.GetRecord(s_trackingSpells.m_data[i],
                &spell)) {
            Spell_C_CancelAura(&spell, ClntObjMgrGetActivePlayerObj(), -1);
        }
    }
}

static int32_t Script_GetSoloWowTrackingMode(lua_State* L) {
    lua_pushstring(L, "multiple");
    return 1;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "GetSoloWowTrackingMode", &Script_GetSoloWowTrackingMode },
};

void SoloWowTracking::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

void SoloWowTracking::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
