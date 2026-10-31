// Witchcraft (M4): Nana's Samhain, kept the way RESEARCH.md describes it.
// Four family lanterns lit for people who have died (a window candle is an
// Irish All Souls' custom Nana took into her practice [S12]); Nana's cider,
// brewed at the iron pot from two apples from the orchard, a sprig of rosemary
// from her herb bed ("for remembrance" [S16]) and a cinnamon stick bought with
// candy at Abbas's Market; Smoke, Nana's old cat, found by the cat and led up
// Lantern Hill; and the offering of cider and an apple at the stones, made with
// Nana. At midnight, with the lanterns lit and the offering made, Smoke goes
// home. Smoke being seen, and following the cat, is the game's fairy tale, not
// a claim about Wiccan belief (RESEARCH.md, sensitivity notes).
//
// Everything here is deterministic and lives in the World; nothing fails if
// the player is late: the shop sells after hours to a girl short of candy for
// a joke, and Smoke goes home whenever everything is ready after midnight.
#pragma once

#include <string>
#include <vector>

#include "sim/world.h"

namespace familiar {

enum LanternId : int { NanaWindow = 0, OkaforPorch, HaleStep, LychLantern };

struct LanternSpot {
    const char* name;     // "Nana's window lantern"
    const char* forWhom;  // "for Nana's mother, Ada"
    Vec3 at;              // the jar, in the world
    Vec2 stand;           // where to stand to light it
};
const LanternSpot& lantern(int i);
int lanternsLit(const World& w);

constexpr float kReach = 1.6f;          // a tree, from where Wren stands
constexpr float kLanternReach = 2.3f;   // a lantern jar (0.1.0 playtest: 1.6 m felt like touching it)
constexpr float kHintRange = 8.0f;      // "come closer": something usable this far off, straight ahead
constexpr float kPotReach = 1.9f;       // the pot, from beside the fire pit's stones
constexpr float kHerbReach = 1.8f;
constexpr float kOfferReach = 2.6f;     // the middle of the stone circle
constexpr int kCinnamonPrice = 3;       // candies, at Abbas's Market
constexpr int kStirs = 4;               // to bring the cider on
constexpr int kApplesWanted = 2;        // one for the pot, one for the offering

// Smoke: where her trail ends, how close the cat must come, how far behind she walks.
constexpr Vec2 kSmokeStart{60.0f, -24.9f};  // under the old oak, across the creek
constexpr float kSmokeFind = 2.0f;
constexpr float kSmokeBehind = 1.2f;
// Her paw prints, from Nana's herb bed to the oak: a polyline to draw them along.
const std::vector<Vec2>& smokeTrail();

// Where Smoke is to be drawn, if she can be seen: a cat sees her all night;
// at the stones, once the offering is made, Wren does too; at midnight she
// walks out of the circle into the dark. False when she is not to be shown.
bool smokeShown(const World& w, Vec3& at, float& yaw);

// Nana walks up Lantern Hill at half past ten and waits at the stones.
constexpr int kNanaLeaves = 22 * 60 + 30;
bool nanaAtCircle(const World& w);
// The minute Nana reaches the stones (for the HUD's "about 11:25").
int nanaArrives();

// Whether Wren has the lantern jar and the basket now: not as a cat, and not
// while her bag (with the jar and basket in it) lies where she became one.
bool carrying(const World& w);

// The lantern Wren could light from here (in reach, unlit), or -1.
int lanternHere(const World& w);
// Whether an apple tree is within reach.
bool atAppleTree(const World& w);
bool atHerbBed(const World& w);
bool atPot(const World& w);
bool atMarket(const World& w);  // on the market's doorstep
bool atCircle(const World& w);  // in the middle of the stones
// Abbas's Market sells once the door has been knocked at (or the porch light is out).
bool shopOpen(const World& w);
// At the shop without the price of a cinnamon stick: a joke (T) will do.
bool haggling(const World& w);

// What E (or T) does here for the night's witchcraft, or "" to leave it to the doors.
std::string craftPrompt(const World& w);
// When nothing is in reach: what Wren is looking straight at that she could
// use from closer (an unlit lantern she can light, a door that would answer),
// as "... come closer", or "". Read from the view (yaw), not the rules.
std::string reachHint(const World& w);

// One word for the touch button that does E here ("Light", "Pick", "Buy",
// "Stir", "Offer"), or "" when E is the doors' (a knock).
std::string craftWord(const World& w);

// The HUD's lines for the night's witchcraft.
std::string lanternLine(const World& w);  // "2 of 4"
std::string basketLine(const World& w);   // "2 apples, rosemary" / "Nana's cider" / "empty"
std::string smokeLine(const World& w);    // "not found yet" ...
// The NEXT line: the night's witchcraft, falling back to the doors' nextStep.
std::string tonightNext(const World& w);

// What the last deed said, for the message line, or "" (shown once by the session).
std::string deedText(const World& w);

// Called by tick before the doors. Lights a lantern, picks an apple or a
// sprig, buys cinnamon, stirs the pot, makes the offering, moves Smoke, and
// sends her home at midnight. True when it used this tick's E or T.
bool craftTick(World& w, const Input& in);

// Where to stand for the browser smoke test's page hook (familiar.toSpot):
// 0 Nana's window, 1 the Okafors' lantern, 2 the herb bed, 3 the pot, 4 an
// apple tree, 5 the market's doorstep, 6 the stone circle, 7 Smoke's oak.
constexpr int kSpotCount = 8;
Vec2 spotStand(int spot);
Vec2 spotFace(int spot);

}  // namespace familiar
