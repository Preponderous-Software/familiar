// The autopilot: a stand-in player that plays the whole night from the World
// alone, from a new game to Smoke going home at midnight.
//
// It chooses a costume on Nana's walk and lights Nana's window lantern; knocks
// at every lit porch along Elm Street from west to east, telling a joke
// wherever a neighbour asks for a trick, lighting Grandpa Ike's lantern after
// the Okafors' door and buying a cinnamon stick at Abbas's Market; becomes the
// cat by the lychgate, slips through the hedge gap and draws the bolt, and
// lights the lychgate lantern; picks two apples in the orchard; becomes the
// cat by Mr. Hale's gate, climbs the woodpile and the fence and noses up the
// latch, lights June Hale's lantern and knocks at his door; picks rosemary and
// brews Nana's cider at the iron pot; leaves the bag at the stones on Lantern
// Hill and, as the cat, finds Smoke under the old oak and leads her into the
// circle; takes up the bag again, makes the offering with Nana, and waits
// with them for midnight, when Smoke goes home. That is the pinned
// playthrough.
//
// It walks a fixed graph of the town's ways (the street, the side passages,
// the cat's way over Mr. Hale's fence and through the hedge, the bridge and
// the hill path), steering along the nearest stretch of the shortest way to
// where it is going. It keeps no memory between ticks: what it does next is a
// function of the World, so a saved game it loads plays on exactly as if it
// had never stopped (tests/test_saves.cpp). Deterministic: the same world
// always gets the same input, on every compiler (detAtan2, never std::atan2).
#pragma once

#include <cstdint>

#include "sim/world.h"

namespace familiar {

// The costume the autopilot wears: Ms. Bright likes a ghost.
constexpr Costume kAutopilotCostume = Costume::Ghost;

// This tick's input for the world as it is.
Input autopilot(const World& w);

// What the autopilot is about, for the HUD and the headless run's log:
// "choosing a costume", "knocking at the Okafors'", "finding Smoke", ...
const char* autopilotDoing(const World& w);

// The house the autopilot would knock at next, or -1 when every lit porch
// with candy has been visited.
int autopilotTarget(const World& w);

// The pinned playthrough ends here: Smoke has gone home.
bool playthroughDone(const World& w);

// The fingerprint the playthrough ends on with seed 1 (see
// tests/test_autopilot.cpp and familiar --headless --autopilot).
constexpr uint64_t kPlaythroughFingerprint = 0x4a059cf194b6ac71ULL;

// The playthrough's bound, in ticks: Smoke goes home at midnight, six hours
// and forty minutes of the night (36,000 ticks) after it starts.
constexpr int64_t kPlaythroughLimit = 60LL * 60 * kTickRate;

// The autopilot's ways, for tests/test_autopilot.cpp: how many stretches
// there are, and whether stretch i can be walked straight from end to end by
// the body it is for (with gates open, for those through a gate).
int autopilotWayCount();
bool autopilotWayWalkable(int i);

}  // namespace familiar
