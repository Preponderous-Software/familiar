// The other people out tonight: children trick-or-treating up and down Elm
// Street in costume, and Nana on her porch. Where they are is a function of
// the clock alone, so they need no state and change nothing in the World
// (they walk through the player rather than block them).
#pragma once

#include <vector>

#include "sim/world.h"

namespace familiar {

struct Walker {
    Vec3 at;        // feet
    float yaw = 0;  // facing; yaw 0 faces -z
    int look = 0;   // which costume (Costume), for the scene
    float height = 1.3f;
    bool shown = false;
};

constexpr int kChildCount = 7;
// The children out in the street: shown from the first porch lights until
// trick-or-treating ends.
std::vector<Walker> children(const World& w);

// Nana: on her porch until half past ten, then along Elm Street, over the
// bridge and up the hill path to the stones, where she waits (M4).
Walker nana(const World& w);
// Whether Nana has reached the stones, and the minute she does.
bool nanaArrived(const World& w);
int nanaArrivalMinute();

}  // namespace familiar
