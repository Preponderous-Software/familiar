// The autopilot: a stand-in player that plays the night from the World alone.
// It chooses a costume on Nana's walk, knocks at every lit porch along Elm
// Street from west to east, tells a joke wherever a neighbour asks for a
// trick, and walks home to Nana's porch, which is the pinned playthrough.
//
// It keeps no memory between ticks: what it does next is a function of the
// World, so a saved game it loads plays on exactly as if it had never
// stopped (tests/test_saves.cpp). Deterministic: the same world always gets
// the same input, on every compiler (detAtan2, never std::atan2).
#pragma once

#include <cstdint>

#include "sim/world.h"

namespace familiar {

// The costume the autopilot wears: Ms. Bright likes a ghost.
constexpr Costume kAutopilotCostume = Costume::Ghost;

// This tick's input for the world as it is.
Input autopilot(const World& w);

// What the autopilot is about, for the HUD and the headless run's log:
// "choosing a costume", "knocking at the Okafors'", "walking home", "home".
const char* autopilotDoing(const World& w);

// The house the autopilot is making for, or -1 when every lit porch with
// candy has been visited (then it makes for Nana's).
int autopilotTarget(const World& w);

// The pinned playthrough ends here: every porch with candy knocked at, and
// Wren home on Nana's doorstep.
bool playthroughDone(const World& w);

// The fingerprint the playthrough ends on with seed 1 (see
// tests/test_autopilot.cpp and familiar --headless --autopilot).
constexpr uint64_t kPlaythroughFingerprint = 0x8953c8ed48851dddULL;

// The playthrough's bound, in ticks: it is done long before this.
constexpr int64_t kPlaythroughLimit = 60LL * 60 * kTickRate;

}  // namespace familiar
