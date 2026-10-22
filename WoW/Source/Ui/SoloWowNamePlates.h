#ifndef WOW_SOURCE_UI_SOLOWOWNAMEPLATES_H
#define WOW_SOURCE_UI_SOLOWOWNAMEPLATES_H

// solo-wow addition, not part of the original client.
//
// Nameplate units, as in later clients: every unit with a nameplate gets a unit token, nameplate1 to nameplate100,
// that works wherever a unit token does (UnitHealth("nameplate3"), macro conditions, unit events such as UNIT_HEALTH
// and UNIT_AURA). Addons follow them with:
//
//   NAME_PLATE_UNIT_ADDED (unit)     a nameplate appeared; the token is valid from here
//   NAME_PLATE_UNIT_REMOVED (unit)   a nameplate is going away; the token is still valid during the event
//   GetNamePlateForUnit(unit)        -> the nameplate frame of any unit token, or nil
//   GetNamePlates()                  -> a table of the nameplate frames with a token
//
// Each nameplate frame also carries its token in frame.namePlateUnitToken while it has one.

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class CGUnit_C;

namespace SoloWowNamePlates {

// A unit's nameplate was created (CGUnit_C::NamePlateAddOrUpdate)
void OnAdded(CGUnit_C* unit);

// A unit's nameplate is about to go away (CGUnit_C::DestroyNamePlate, CGUnit_C::Disable)
void OnRemoved(CGUnit_C* unit);

// Every nameplate is being cleared at UI shutdown; no events
void Clear();

// The unit behind nameplate token number index + 1, or an empty GUID
WOWGUID GetUnit(uint32_t index);

// The token number - 1 of a unit's nameplate, or -1 when it has none
int32_t GetIndex(const WOWGUID& guid);

void RegisterScriptFunctions();
void UnregisterScriptFunctions();

}

#endif
