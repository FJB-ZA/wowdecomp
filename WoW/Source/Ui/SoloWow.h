#ifndef WOW_SOURCE_UI_SOLOWOW_H
#define WOW_SOURCE_UI_SOLOWOW_H

// solo-wow addition, not part of the original client.
//
// The solo-wow server (scripts/Custom/client_status.cpp and spec_slots.cpp in its core fork) sends the player's
// progression step, personal XP rate and spec slots as hidden addon whispers to the player themself. The stock
// client hands those to addons as CHAT_MSG_ADDON. This client keeps them instead, so the data is there before any
// addon loads and survives /reload, and offers it to Lua as functions and events:
//
//   GetSoloWowProgression()     -> step (0-18)                  event SOLOWOW_PROGRESSION_UPDATE
//   GetSoloWowXPRate()          -> rate, maxRate, paused (1/nil) event SOLOWOW_XP_RATE_UPDATE
//   GetNumSpecSlots()           -> owned, active, nextPriceCopper event SOLOWOW_SPEC_SLOTS_UPDATE
//   GetSpecSlotTalents(slot)    -> points in tree 1, 2, 3, then the slot's sub-class path name and id (if any)
//
// Each returns nil until the server has sent that part. Addons can test for the functions and keep their
// CHAT_MSG_ADDON path for the stock client.

#include <cstdint>

namespace SoloWow {

// Takes an addon message whispered to the active player by themself. Returns true when it was ours.
bool HandleAddonMessage(const char* prefix, const char* message);

void RegisterScriptFunctions();
void UnregisterScriptFunctions();

}

#endif
