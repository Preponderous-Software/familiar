// The town's ways: a fixed graph of places (the street, the front walks, the
// side passages, the gates, the cat's way over Mr. Hale's fence and through
// the hedge gap, the bridge and the hill path) and the stretches between them.
// The autopilot walks it (sim/autopilot.h), and the NEXT marker follows it
// (#25), so neither leads Wren into a wall, a fence or the creek.
//
// Deterministic, and a function of the World alone.
#pragma once

#include <cmath>

#include "sim/world.h"

namespace familiar::ways {

enum Node : int {
    // Elm Street, along the north lane (z = -2.5), west to east.
    SNana, SPassage, SGarcia, SOkafor, SHalePassage, SBright, SHale, SNovak, SLindqvist, SMarket, SPell, SWhitfield, SEast,
    // Doorsteps (no Nana: she is at home).
    DOkafor, DHale, DLindqvist, DPell, DGarcia, DBright, DNovak, DMarket, DWhitfield,
    // Nana's: her front walk (the start), her window lantern; her back yard by the side passage.
    Start, NanaWindow_, Passage, HerbBed, Pot,
    OkaforLantern,
    // Mr. Hale's: the passage west of his yard, outside and inside his gate, his back step.
    HaleBack, HaleGateOut, HaleGateIn, HaleYard, HaleStep_,
    // The cat's way round his yard, onto the woodpile, over the fence, to the latch.
    CatHaleNW, CatHaleNE, CatWoodpile, CatInYard, CatHaleLatch,
    // The churchyard: the green north of the street, the lychgate (outside and in).
    Green, Lychgate, LychInside,
    // The cat's way round to the hedge gap and in.
    CatPellBack, CatHedgeOut, CatHedgeIn, CatChurchyard,
    // Over the bridge; the orchard; up the hill path to the stones.
    BridgeWest, BridgeEast, HillFoot, Orchard, HillPath, StonesGap, Circle,
    // The cat's way to the old oak, where Smoke's trail ends.
    CatWoods, CatCreekside, CatOak,
    NodeCount
};

extern const Vec2 kNodes[NodeCount];

constexpr int kGirl = 1, kCat = 2, kBoth = 3;

struct Way {
    int a, b;
    int forms;        // who walks it (a girl's way is a cat's too)
    int gate = -1;    // only while this gate is open
    bool oneWay = false;  // a to b only (down off a fence)
};

extern const Way kWays[];
extern const int kWayCount;

constexpr float kSample = 0.2f;  // how finely a straight walk is checked

inline float dist2(Vec2 a, Vec2 b) { return (a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z); }
inline float dist(Vec2 a, Vec2 b) { return std::sqrt(dist2(a, b)); }

int formBit(Form f);
// Whether the World's body may walk this stretch now: its form, its gate.
bool usable(const World& w, const Way& e);

// Whether a body of `form`, standing at `from` with its feet at `feet`, could
// walk straight to `to`: every step on the way somewhere it may stand.
bool straightWalk(const World& w, Form form, Vec2 from, float feet, Vec2 to);

// The shortest way from every place to `goal` over the ways this body may
// walk now: how far, and the next place on it (-1: no way).
struct Routes {
    float far[NodeCount];
    int next[NodeCount];
};

Routes routesTo(const World& w, int goal);

// The place nearest a spot.
int nearestPlace(Vec2 s);

}  // namespace familiar::ways
