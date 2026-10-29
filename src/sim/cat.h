// The cat (M3): what changes when Wren is a cat, beyond climbing and gaps
// (those are in world.cpp's standAt). A cat cannot carry the candy bag, so it
// stays on the ground where Wren changed until she picks it up; a cat noses
// up latches from the inside that a girl cannot reach; and a cat sees the
// spirits out walking on the one night the veil is thin. The spirits and the
// cat form are the game's fairy tale, not Wiccan belief (RESEARCH.md,
// sensitivity notes), and every spirit here is gentle: the town's own dead,
// visiting.
#pragma once

#include <string>
#include <vector>

#include "sim/npc.h"
#include "sim/world.h"

namespace familiar {

constexpr float kBagReach = 1.4f;     // Wren picks up her bag from this close
constexpr float kLatchReach = 1.3f;   // the cat reaches a latch from this close, inside
constexpr float kSpiritSight = 7.0f;  // the cat notices a spirit this close

// Whether Wren (as a girl) is within reach of her dropped bag.
bool nearBag(const World& w);

// The gate whose latch the cat can open from here (inside, within reach, shut), or -1.
int latchHere(const World& w);
// The gate the player stands outside of while it is shut, or -1.
int latchedOutside(const World& w);

// The spirits out tonight: three in the churchyard and the lamplighter on the
// hill path. Where they are is a function of the clock; only a cat sees them.
enum SpiritId : int { Reader = 0, Farmer, Fiddler, Lamplighter, SpiritCount };
std::vector<Walker> spirits(const World& w);
// The nearest spirit the cat sees within kSpiritSight, or -1 (always -1 for Wren).
int spiritNear(const World& w);
// What the cat sees of spirit i, for the message line.
const char* spiritText(int i);
int spiritsSeen(const World& w);  // how many different spirits the cat has seen
// Where the lamplighter's lamp hangs, for the light (shown only to the cat).
Vec3 lamplighterLamp(const World& w);

// What E does here because of the cat or the bag (picking it up, a latch, a
// shut gate), or "" to leave it to the doors.
std::string catPrompt(const World& w);
// One word for the touch button that does E here: "Pick up", "Open", "Meow" or "Knock".
std::string useLabel(const World& w);

// Called by tick after the form may have changed (`before` is the form at the
// start of the tick). Drops the bag on becoming a cat, picks it up on becoming
// Wren beside it or on E, opens a latch on E, and notes the spirits seen.
// True when it used this tick's E, so the doors should not see it.
bool catTick(World& w, const Input& in, Form before);

}  // namespace familiar
