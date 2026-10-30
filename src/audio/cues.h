// What Hollowell sounds like on Halloween night, worked out from the world:
// the steady sounds (the wind and the dry leaves, the creek, an owl after
// dark, footsteps, a purr while Smoke is close) and the one-off sounds of
// things happening (a knock and the door opening, candy in the bag, a meow,
// a latch, a lantern lit, the cider stirred, the church bell on the hour,
// Smoke going home at midnight). No device and no engine: the synth
// (synth.h) turns these into samples, and the tests can check them without
// either. Everything is gentle: this is a game for a family. (Atman's
// audio, made for an autumn night.)
#pragma once

#include <vector>

#include "sim/world.h"

namespace familiar::audio {

// What the footsteps fall on.
enum class Ground : int { Leaves = 0, Road, Wood };

// The steady sounds, each 0 (silent) to 1 (full).
struct Ambience {
    float wind = 0;    // always a little, more up on Lantern Hill
    float leaves = 0;  // dry leaves stirring, more under the trees
    float creek = 0;   // near the water
    float owl = 0;     // after dark, more toward the woods
    float purr = 0;    // Smoke close by
    float steps = 0;   // footsteps per second
    Ground ground = Ground::Leaves;
    bool cat = false;  // a cat's soft pads, not a girl's shoes
};

// One-off sounds.
enum class Cue : int {
    Knock,     // at a door, and it opening
    Candy,     // candy into the bag
    Meow,      // the cat: on becoming one, and at a door
    Change,    // Wren becomes the cat
    ChangeBack,
    Bag,       // the bag set down or picked up
    Latch,     // a gate's latch or bolt
    Lantern,   // a match struck, a lantern lit
    Pick,      // an apple or a sprig of rosemary
    ShopBell,  // the market's door bell: cinnamon bought
    Stir,      // the iron pot
    Brewed,    // the cider ready
    Spirit,    // the cat meets a spirit
    Page,      // a new page (or a secret) in Wren's notebook
    Found,     // Smoke found: her own small meow
    Settle,    // Smoke sits in the ring of stones, purring
    Offering,  // the offering made with Nana
    Home,      // Smoke goes home at midnight
    Bell,      // one stroke of the church clock
    Count
};

// `listener` is where the ears are (the eyes); `stepsPerSecond` is how fast
// the body is walking, 0 standing.
Ambience ambience(const World& w, Vec3 listener, float stepsPerSecond);

// The one-off sounds for the change from `before` to `after` (one frame).
// The church clock strikes the hour (its number of strokes; twelve at
// midnight) as the night passes it. Nothing for a jump (a load, a new night).
std::vector<Cue> cues(const World& before, const World& after);

}  // namespace familiar::audio
