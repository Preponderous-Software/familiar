// The whole game state, and the one function that advances it. No engine
// types: the simulation builds and tests without a GPU, and the same inputs
// always give the same world (see tests/test_determinism.cpp).
#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "sim/math.h"
#include "sim/town.h"

namespace familiar {

constexpr int kTickRate = 30;                       // ticks per second of play
constexpr double kGameMinutesPerSecond = 1.0 / 3;   // one game minute every three seconds of play
constexpr int kStartMinute = 17 * 60 + 20;          // 5:20 pm on 31 October
constexpr int kMidnight = 24 * 60;

// Wren, or the cat she can become (from M3).
enum class Form : int32_t { Girl = 0, Cat = 1 };

struct Player {
    float x = 0, z = 0;
    float feet = 0;            // the height she stands at: the ground, or something she climbed onto
    float yaw = 0, pitch = 0;  // yaw 0 faces north (-z), positive turns right
};

struct World {
    uint64_t seed = 0;
    int64_t tick = 0;
    Player player;
    Form form = Form::Girl;
    std::array<bool, town::GateCount> gateOpen{};
    int32_t candy = 0;
    std::array<bool, town::HouseCount> knocked{};  // each house answers once
    int32_t answered = -1;      // the house whose door last opened, or -1
    int64_t answeredTick = -1;  // and when
    bool kitty = false;         // the answer was to a cat (a treat for the kitty, no candy)
};

// One tick's worth of player intent, already resolved from keys and mouse.
struct Input {
    float forward = 0;  // -1..1, + walks the way the player faces
    float strafe = 0;   // -1..1, + steps right
    bool hurry = false;
    float yaw = 0, pitch = 0;  // absolute view angles for this tick
    bool use = false;          // knock, or whatever E does here
    bool toggleForm = false;   // Q: Wren to cat, or back
};

World newWorld(uint64_t seed);
void tick(World& w, const Input& in);

// How the body moves, by form.
struct Body {
    float radius;  // for collision
    float climb;   // the highest step up onto something, in metres
    float eye;     // eye height above the feet
    float walk, hurry;  // metres per second
};
const Body& body(Form f);

// Where a body of `form` at height `feet` could stand at (x, z): true and the
// height it would stand at, or false if something there is too tall to climb
// onto, or it is the creek. Gates count while shut.
bool standAt(const World& w, Form form, float x, float z, float feet, float& out);

// Whether the cat may become Wren again here: on the ground, with room for a girl.
bool canBecomeGirl(const World& w);

// The eye: the feet plus the form's eye height.
Vec3 eyePosition(const World& w);

// The game clock: minutes since midnight at the start of 31 October (passes 1440 after midnight).
double gameMinutes(const World& w);
// "5:20 pm", for the HUD.
std::string clockText(const World& w);

// A hash of everything in the World, for determinism tests and playthroughs.
uint64_t fingerprint(const World& w);

}  // namespace familiar
