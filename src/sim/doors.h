// Trick-or-treating: porch lights, knocking, and what each neighbour gives.
// A house with its porch light on answers a knock once; a cat at the door
// gets a treat for the kitty and no candy.
#pragma once

#include <string>

#include "sim/world.h"

namespace familiar {

constexpr int kPorchLightsOff = 21 * 60;  // trick-or-treating ends at 9 pm
constexpr float kDoorReach = 1.6f;        // from the doorstep

struct Neighbour {
    const char* who;
    int candy;         // given to Wren
    const char* says;  // when they open the door to her
};
const Neighbour& neighbour(town::HouseId h);

// Whether the house's porch light is on now.
bool porchLightOn(const World& w, town::HouseId h);

// The house whose doorstep the player is at, or -1.
int atDoor(const World& w);

// What E does here, in words, for the prompt; empty if nothing.
std::string usePrompt(const World& w);

// What the door said, for the message line, while the answer shows; empty otherwise.
std::string answerText(const World& w);
constexpr float kAnswerSeconds = 6.0f;  // the neighbour stands in the doorway this long

// The HUD's one line on what to do next.
std::string nextStep(const World& w);

// One tick of doors; tick() calls it.
void doorsTick(World& w, const Input& in);

}  // namespace familiar
