#ifndef WOW_SOURCE_UI_SOLOWOWTRACKING_H
#define WOW_SOURCE_UI_SOLOWOWTRACKING_H

// solo-wow addition, not part of the original client.
//
// Minimap tracking as in retail: several kinds at once. The original client keeps one town filter
// (CGMinimapFrame::s_trackingInfo: repair, flight master, mailbox, ...) and one tracking spell, and choosing either
// drops the other. Here any number of town filters can be on together, and they stay on beside the tracking spells,
// which the solo-wow server lets stack (herbs, minerals and a creature type at once).
//
//   GetTrackingInfo(index)     -> active for every filter that is on and every tracking spell whose aura is on
//   SetTracking(index)         toggles that filter or spell (an active spell's aura is cancelled)
//   SetTracking()              turns everything off
//   GetSoloWowTrackingMode()   -> "multiple", so addons know they can offer checkboxes
//
// CGMinimapFrame::s_trackingInfo stays the filter the tracking button shows (the newest one on), and
// CGMinimapFrame::s_trackingSpell the tracking spell it shows. The filters on are saved in the minimapTrackedInfo
// CVar, space separated.

#include <cstdint>

struct TrackingTypeInfo;

namespace SoloWowTracking {

// Everything off, at the start of a game session (CGMinimapFrame::InitializeGame)
void Reset();

// Whether a town filter is on, and turning one on or off; Clear turns them all off
bool IsOn(const TrackingTypeInfo* info);
void Set(const TrackingTypeInfo* info, bool on);
void Clear();

// The town filter to show on the tracking button: info when it's on, else the first one on, else nullptr
const TrackingTypeInfo* ShownType(const TrackingTypeInfo* info);

// What the filters on track: NPC flags (type 1), whether a gameobject type (type 2) and trivial quests (type 3)
uint32_t NpcFlags();
bool TracksGameObjectType(uint32_t typeId);
bool TracksTrivialQuests();

// The filters on as the minimapTrackedInfo CVar holds them (space separated names)
void FormatState(char* buffer, uint32_t size);

// Whether the active player has a tracking spell's aura; a tracking spell on other than `except`, or 0
bool HasSpell(int32_t spellId);
int32_t AnySpellOnExcept(int32_t except);

// Cancels every tracking spell aura on the active player
void CancelSpells();

void RegisterScriptFunctions();
void UnregisterScriptFunctions();

}

#endif
