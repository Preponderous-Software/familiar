// Trick-or-treating: costumes, porch lights, knocking, tricks, and what each
// neighbour gives. A house with its porch light on answers a knock; some
// neighbours ask for a trick first (a joke, as children guising in Scotland
// and Ireland told [S20]), and some are partial to a costume. Each house gives
// candy once. A cat at the door gets a treat for the kitty and no candy.
#pragma once

#include <string>

#include "sim/world.h"

namespace familiar {

constexpr int kPorchLightsOff = 21 * 60;  // trick-or-treating ends at 9 pm
constexpr float kDoorReach = 1.6f;        // from the doorstep
constexpr float kCostumeReach = 9.0f;     // costumes are chosen near Nana's porch
constexpr int kJokeCount = 8;

const char* costumeName(Costume c);   // "a ghost", ...
const char* costumeShort(Costume c);  // "ghost", ...

struct Neighbour {
    const char* who;
    int candy;            // given to Wren
    bool asksTrick;       // asks for a trick first
    Costume likes;        // one more candy for this costume
    const char* treat;    // what they say as they give it
    const char* liked;    // what they say about the costume they like
    const char* ask;      // asking for a trick
    const char* laugh;    // after the joke
};
const Neighbour& neighbour(town::HouseId h);

// The jokes Wren knows, in the order she tells them.
const char* joke(int i);

bool porchLightOn(const World& w, town::HouseId h);
int atDoor(const World& w);
bool canChooseCostume(const World& w);

std::string usePrompt(const World& w);

// What the door said, for the message line, while the answer shows; empty otherwise.
std::string answerText(const World& w);
constexpr float kAnswerSeconds = 7.0f;

// Houses still to knock at while their porch lights are on.
int housesLeft(const World& w);

std::string nextStep(const World& w);

void doorsTick(World& w, const Input& in);

}  // namespace familiar
