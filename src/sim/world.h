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

// Wren, or the cat she can become.
enum class Form : int32_t { Girl = 0, Cat = 1 };

// What Wren wears trick-or-treating (chosen on Nana's walk, 1-4).
enum class Costume : int32_t { None = 0, Ghost = 1, Pumpkin = 2, Skeleton = 3, Witch = 4 };

// How a door was last answered.
enum class Answer : int32_t { None = 0, Treat, Liked, Ask, Joke, Kitty };

// The four family lanterns (M4, sim/craft.h).
constexpr int kLanternCount = 4;

// Smoke, Nana's old cat, who died this spring (M4, sim/craft.h).
enum class SmokeState : int32_t {
    Lost = 0,   // by the old oak, where her trail ends
    Following,  // behind the cat
    Waiting,    // sat down where Wren stopped being a cat
    AtCircle,   // at the stones on Lantern Hill
    Home        // gone home at midnight
};

// The last thing the night's witchcraft did, for the message line (sim/craft.h).
enum class Deed : int32_t {
    None = 0, Lit, Apple, Rosemary, Bought, BoughtJoke, Stir, Brewed, Found, Waits, Circle, Offered, Home
};

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
    Answer answer = Answer::None;
    int32_t asking = -1;        // the house waiting for a trick, or -1
    int32_t jokes = 0;          // jokes told so far
    Costume costume = Costume::None;
    // The candy bag: a cat cannot carry it, so it stays on the ground where
    // Wren changed until she picks it up (M3). The candy stays counted.
    bool bagDown = false;
    Vec3 bag;                    // where it lies (feet height in y)
    uint32_t spiritsSeen = 0;    // bit i: the cat has seen spirit i (sim/cat.h)

    // Witchcraft (M4): the family lanterns, the cider's ingredients, Smoke and
    // the offering, and Wren's notebook.
    std::array<bool, kLanternCount> lit{};
    int32_t apples = 0;       // carried: one for the pot, one for the offering
    bool rosemary = false;
    bool cinnamon = false;
    int32_t stirs = 0;        // at the pot
    bool cider = false;       // brewed
    bool offered = false;     // the offering made at the stones
    SmokeState smoke = SmokeState::Lost;
    float smokeX = 0, smokeZ = 0;  // where Smoke is
    int64_t homeTick = -1;    // when Smoke went home
    uint32_t pages = 0;       // bit i: notebook page i (sim/notebook.h)
    uint32_t secrets = 0;     // bit i: secret i found
    Deed deed = Deed::None;   // the last deed,
    int32_t deedArg = 0;      // what it was done to (a lantern, a stir),
    int64_t deedTick = -1;    // and when
};

// One tick's worth of player intent, already resolved from keys and mouse.
struct Input {
    float forward = 0;  // -1..1, + walks the way the player faces
    float strafe = 0;   // -1..1, + steps right
    bool hurry = false;
    float yaw = 0, pitch = 0;  // absolute view angles for this tick
    bool use = false;          // knock, pick up the bag, nose up a latch: whatever E does here
    bool toggleForm = false;   // Q: Wren to cat, or back
    bool trick = false;        // T: tell a joke when a neighbour asks for a trick
    int32_t costume = 0;       // 1-4: choose a costume on Nana's walk
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
