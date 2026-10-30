// The autopilot plays the whole night from a new game: a ghost costume on
// Nana's walk and her window lantern; every lit porch with candy from west to
// east, a joke at the Okafors' and at Ms. Bright's, Grandpa Ike's lantern and
// a cinnamon stick on the way; the cat through the hedge gap to draw the
// lychgate's bolt, and the lychgate lantern; apples from the orchard; the cat
// over Mr. Hale's fence to his latch, June's lantern and his door; rosemary
// and the cider; Smoke found and led to the stones; the offering with Nana;
// and Smoke going home at midnight. It ends on the pinned fingerprint on
// every compiler, and as the World alone decides what it does, it plays the
// same from any point (see also tests/test_saves.cpp).
#include <cinttypes>
#include <cstdio>
#include <string>

#include "check.h"
#include "sim/autopilot.h"
#include "sim/cat.h"
#include "sim/craft.h"
#include "sim/doors.h"
#include "sim/world.h"

using namespace familiar;
namespace T = familiar::town;

namespace {

// Plays on to the end; false if the autopilot ever walks into something and
// stays stuck for a second, or never finishes.
bool playOn(World& w, int64_t* stuckFor = nullptr) {
    int64_t stuck = 0, longest = 0;
    while (!playthroughDone(w) && w.tick < kPlaythroughLimit) {
        const Input in = autopilot(w);
        const float x = w.player.x, z = w.player.z;
        tick(w, in);
        // It may stand and wait (for Nana, for midnight), but never walks on the spot.
        if (in.forward != 0 && w.player.x == x && w.player.z == z) longest = ++stuck > longest ? stuck : longest;
        else stuck = 0;
    }
    if (stuckFor) *stuckFor = longest;
    return playthroughDone(w) && longest < kTickRate;
}

// The whole night done: everything the MVP's night asks of it.
bool wholeNight(const World& w) {
    return w.form == Form::Girl && !w.bagDown && lanternsLit(w) == kLanternCount && w.gateOpen[T::HaleGate] &&
           w.gateOpen[T::Lychgate] && w.cider && w.offered && w.smoke == SmokeState::Home && w.knocked[T::Hale] &&
           autopilotTarget(w) < 0;
}

}  // namespace

int main(int, char**) {
    // Every stretch of the autopilot's ways can be walked straight, by the body it is for.
    for (int i = 0; i < autopilotWayCount(); ++i) {
        if (!autopilotWayWalkable(i)) std::printf("way %d cannot be walked\n", i);
        CHECK(autopilotWayWalkable(i));
    }

    World w = newWorld(1);
    CHECK(!playthroughDone(w));
    CHECK(autopilotTarget(w) == T::Garcia);  // the westernmost lit porch with candy
    CHECK(std::string(autopilotDoing(w)) == "choosing a costume");

    // Each step of the night, in order, as the autopilot names it.
    const char* const kSteps[] = {
        "lighting Nana's window lantern", "knocking at the Garcias'", "knocking at the Okafors'", "lighting Grandpa Ike's lantern",
        "knocking at Abbas's Market", "buying a cinnamon stick", "knocking at the Whitfields'", "going to the lychgate to be a cat",
        "slipping into the churchyard to draw the bolt", "lighting the lychgate lantern", "picking apples in the orchard",
        "going to Mr. Hale's gate to be a cat", "climbing in to Mr. Hale's latch", "lighting June Hale's lantern",
        "knocking at Mr. Hale's", "picking rosemary", "brewing Nana's cider", "going up Lantern Hill to be a cat", "finding Smoke",
        "leading Smoke to the stones", "waiting for Nana at the stones", "making the offering with Nana",
        "waiting with Nana and Smoke for midnight", "Smoke has gone home"};
    size_t next = 0;
    int knocks = 0;
    int64_t stuck = 0, longestStuck = 0, catTicks = 0, waitedForNana = 0;
    while (!playthroughDone(w) && w.tick < kPlaythroughLimit) {
        const int before = w.answered;
        const Input in = autopilot(w);
        const float x = w.player.x, z = w.player.z;
        tick(w, in);
        if (w.answered != before && w.answered >= 0) ++knocks;
        if (in.forward != 0 && w.player.x == x && w.player.z == z) longestStuck = ++stuck > longestStuck ? stuck : longestStuck;
        else stuck = 0;
        if (w.form == Form::Cat) ++catTicks;
        const std::string doing = autopilotDoing(w);
        if (doing == "waiting for Nana at the stones") ++waitedForNana;
        if (next < sizeof kSteps / sizeof kSteps[0] && doing == kSteps[next]) ++next;
    }
    CHECK(next == sizeof kSteps / sizeof kSteps[0]);  // every step, in that order
    CHECK(playthroughDone(w));
    CHECK(wholeNight(w));
    CHECK(longestStuck < kTickRate);
    CHECK(w.costume == kAutopilotCostume);
    for (int h = 0; h < T::HouseCount; ++h)  // every porch with candy, Mr. Hale's too once June's lantern lights it
        CHECK(w.knocked[static_cast<size_t>(h)] == (h != T::Nana && neighbour(static_cast<T::HouseId>(h)).candy > 0));
    CHECK(knocks >= 9);
    CHECK(w.jokes == 2);  // the Okafors and Ms. Bright asked for a trick
    // 18 along Elm Street (3 + 3 + 1 + 2 + (2 + 1 + 1 for the ghost) + 2 + 1 + 2), 2 from Mr. Hale, 3 for the cinnamon.
    CHECK(w.candy == 17);
    CHECK(w.apples == 0 && !w.rosemary && !w.cinnamon && w.stirs == kStirs);
    CHECK(w.spiritsSeen != 0);  // the cat met someone on the way
    CHECK(catTicks > 0 && waitedForNana > 0);
    CHECK(gameMinutes(w) >= kMidnight && w.homeTick == w.tick);  // she goes home on the stroke of midnight
    CHECK(atCircle(w));

    const uint64_t fp = fingerprint(w);
    std::printf("playthrough ends at %s (tick %" PRId64 ") with %d candy, %d lanterns, Smoke %s, fingerprint %016" PRIx64 "\n",
                clockText(w).c_str(), w.tick, w.candy, lanternsLit(w), smokeLine(w).c_str(), fp);
    CHECK(fp == kPlaythroughFingerprint);

    // A cat at the start of the night is turned back into Wren first.
    {
        World c = newWorld(1);
        Input q;
        q.yaw = c.player.yaw;
        q.toggleForm = true;
        tick(c, q);
        CHECK(c.form == Form::Cat);
        CHECK(autopilot(c).toggleForm);
        CHECK(playOn(c));
        CHECK(wholeNight(c) && c.candy == 17);
    }

    // The bag dropped on the street by a cat who wandered off: Wren turns back,
    // fetches it, and the night goes on to the same end.
    {
        World c = newWorld(1);
        Input in;
        in.yaw = kPi;  // south, out to the street
        in.toggleForm = true;
        tick(c, in);
        CHECK(c.bagDown);
        in.toggleForm = false;
        in.forward = 1;
        for (int t = 0; t < 2 * kTickRate; ++t) tick(c, in);
        CHECK(c.form == Form::Cat && c.bagDown);
        CHECK(playOn(c));
        CHECK(wholeNight(c) && c.candy == 17);
    }

    // A cat up on Nana's shed gets down to where a girl fits before changing back.
    {
        World c = newWorld(1);
        Input q;
        q.yaw = c.player.yaw;
        q.costume = 1;
        tick(c, q);
        q.costume = 0;
        q.toggleForm = true;
        tick(c, q);
        c.player.x = (T::kNanaShed.x0 + T::kNanaShed.x1) / 2;
        c.player.z = (T::kNanaShed.z0 + T::kNanaShed.z1) / 2;
        c.player.feet = T::kShedHeight;
        CHECK(!canBecomeGirl(c));
        CHECK(playOn(c));
        CHECK(wholeNight(c));
    }

    TEST_MAIN_END();
}
